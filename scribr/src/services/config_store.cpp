#include "config_store.h"

#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "sd_card.h"

namespace services::config {
namespace {
constexpr long MIN_OFFSET_MIN = -1440;
constexpr long MAX_OFFSET_MIN = 1440;

const char* skipSpace(const char* s) {
  while (*s == ' ' || *s == '\t' || *s == '\r') ++s;
  return s;
}

void rtrim(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) {
    s[--n] = 0;
  }
}

bool parseLongStrict(const char* s, long& out) {
  s = skipSpace(s);
  if (*s == 0) return false;
  char* end = nullptr;
  long value = strtol(s, &end, 10);
  end = const_cast<char*>(skipSpace(end));
  if (*end != 0) return false;
  out = value;
  return true;
}

int twoDigits(const char* s) {
  if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1])) return -1;
  return (s[0] - '0') * 10 + (s[1] - '0');
}

int fourDigits(const char* s) {
  for (int i = 0; i < 4; ++i) {
    if (!isdigit((unsigned char)s[i])) return -1;
  }
  return (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0');
}
}  // namespace

bool isValidLocalOffset(long minutes) {
  return minutes >= MIN_OFFSET_MIN && minutes <= MAX_OFFSET_MIN;
}

bool loadBootConfig(BootConfig& out) {
  out = BootConfig{};
  sdcard::begin();
  if (!sdcard::mounted()) return false;

  File f = sdcard::fs().open("/scribr.cfg", FILE_READ);
  if (!f) {
    Serial.println("scribr: /scribr.cfg missing; using config defaults");
    return false;
  }

  const size_t len = f.size();
  if (len == 0 || len > 4096) {
    Serial.println("scribr: /scribr.cfg empty/too large; using config defaults");
    f.close();
    return false;
  }

  char* buf = (char*)malloc(len + 1);
  if (!buf) {
    f.close();
    return false;
  }
  const size_t n = f.readBytes(buf, len);
  f.close();
  buf[n] = 0;
  const bool ok = parseBootConfigText(buf, out);
  free(buf);
  return ok;
}

bool markRtcSetApplied(const char* rtcSetUtc) {
  if (!isValidUtcTimestamp(rtcSetUtc)) return false;
  if (!sdcard::mounted()) return false;

  File in = sdcard::fs().open("/scribr.cfg", FILE_READ);
  if (!in) return false;
  File out = sdcard::fs().open("/scribr.cfg.tmp", FILE_WRITE);
  if (!out) {
    in.close();
    return false;
  }

  bool changed = false;
  while (in.available()) {
    String line = in.readStringUntil('\n');
    bool hadCr = line.endsWith("\r");
    if (hadCr) line.remove(line.length() - 1);

    String trimmed = line;
    trimmed.trim();
    bool applyLine = false;
    if (!trimmed.startsWith("#")) {
      int eq = trimmed.indexOf('=');
      if (eq > 0) {
        String key = trimmed.substring(0, eq);
        String value = trimmed.substring(eq + 1);
        key.trim();
        value.trim();
        applyLine = (key == "rtc_set_utc" && value == rtcSetUtc);
      }
    }

    if (applyLine) {
      out.print("# applied rtc_set_utc=");
      out.print(rtcSetUtc);
      changed = true;
    } else {
      out.print(line);
    }
    if (hadCr) out.print('\r');
    if (in.available()) out.print('\n');
  }
  in.close();
  out.flush();
  out.close();

  if (!changed) {
    sdcard::fs().remove("/scribr.cfg.tmp");
    return true;
  }

  sdcard::fs().remove("/scribr.cfg.bak");
  sdcard::fs().rename("/scribr.cfg", "/scribr.cfg.bak");
  if (!sdcard::fs().rename("/scribr.cfg.tmp", "/scribr.cfg")) {
    if (sdcard::fs().exists("/scribr.cfg.bak")) sdcard::fs().rename("/scribr.cfg.bak", "/scribr.cfg");
    sdcard::fs().remove("/scribr.cfg.tmp");
    return false;
  }
  sdcard::fs().remove("/scribr.cfg.bak");
  Serial.println("scribr: marked rtc_set_utc applied in /scribr.cfg");
  return true;
}

bool isValidUtcTimestamp(const char* value) {
  if (!value || strlen(value) != 20) return false;
  if (value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
      value[13] != ':' || value[16] != ':' || value[19] != 'Z') {
    return false;
  }

  const int year = fourDigits(value);
  const int month = twoDigits(value + 5);
  const int day = twoDigits(value + 8);
  const int hour = twoDigits(value + 11);
  const int minute = twoDigits(value + 14);
  const int second = twoDigits(value + 17);

  if (year < 2024 || year > 2099) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;

  // Keep this intentionally conservative and cheap. RTC/system-clock code will
  // apply final epoch sanity checks when converting to time_t.
  return true;
}

bool parseBootConfigText(const char* text, BootConfig& out) {
  out = BootConfig{};
  if (!text) return false;

  const char* p = text;
  bool sawAnyLine = false;
  while (*p) {
    char line[128] = {0};
    size_t n = 0;
    while (*p && *p != '\n' && n < sizeof(line) - 1) line[n++] = *p++;
    while (*p && *p != '\n') ++p;  // discard overlong remainder
    if (*p == '\n') ++p;
    line[n] = 0;
    rtrim(line);

    char* cur = const_cast<char*>(skipSpace(line));
    if (*cur == 0 || *cur == '#') continue;
    sawAnyLine = true;

    char* eq = strchr(cur, '=');
    if (!eq) continue;
    *eq = 0;
    char* key = cur;
    char* value = const_cast<char*>(skipSpace(eq + 1));
    rtrim(key);
    rtrim(value);

    if (strcmp(key, "local_offset_min") == 0) {
      long parsed = 0;
      if (parseLongStrict(value, parsed) && isValidLocalOffset(parsed)) {
        out.localOffsetMin = (int16_t)parsed;
      }
    } else if (strcmp(key, "rtc_set_utc") == 0) {
      if (isValidUtcTimestamp(value)) {
        out.hasRtcSetUtc = true;
        strncpy(out.rtcSetUtc, value, sizeof(out.rtcSetUtc) - 1);
      }
    }
  }

  return sawAnyLine;
}

}  // namespace services::config
