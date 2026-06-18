#include "clock.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../drivers/rtc_pcf85063.h"

namespace services::clock {
namespace {
Status st;

void setSystemClock(time_t epoch) {
  timeval tv = {};
  tv.tv_sec = epoch;
  settimeofday(&tv, nullptr);
}
}  // namespace

void begin(const services::config::BootConfig& cfg) {
  st = Status{};
  st.localOffsetMin = cfg.localOffsetMin;

  drivers::rtc::begin();

  if (cfg.hasRtcSetUtc) {
    drivers::rtc::DateTimeUtc dt;
    time_t epoch;
    if (drivers::rtc::fromIso8601(cfg.rtcSetUtc, dt) && drivers::rtc::toEpoch(dt, epoch) && drivers::rtc::write(dt)) {
      setSystemClock(epoch);
      st.bootstrapApplied = true;
      st.rtcValid = true;
      st.timeSet = true;
      if (!services::config::markRtcSetApplied(cfg.rtcSetUtc)) {
        Serial.println("scribr: WARNING failed to mark rtc_set_utc applied; stale time may reapply");
      }
      Serial.println("scribr: RTC bootstrap applied from /scribr.cfg");
      return;
    }
  }

  drivers::rtc::DateTimeUtc dt;
  bool os = false;
  time_t epoch;
  if (drivers::rtc::read(dt, os) && !os && drivers::rtc::toEpoch(dt, epoch)) {
    setSystemClock(epoch);
    st.rtcValid = true;
    st.timeSet = true;
    Serial.println("scribr: system clock synced from RTC");
  } else {
    st.rtcValid = false;
    st.timeSet = false;
    Serial.println("scribr: RTC invalid; TIME NOT SET condition pending");
  }
}

Status status() { return st; }

bool nowUtc(time_t& out) {
  if (!st.timeSet) return false;
  time(&out);
  return out >= 1700000000;
}

void formatLocal(time_t utc, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  time_t local = utc + (time_t)st.localOffsetMin * 60;
  tm t;
  gmtime_r(&local, &t);
  snprintf(out, outLen, "%04d-%02d-%02d %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
}

bool setFromEpoch(time_t epoch) {
  if (epoch < 1700000000) return false;  // same sanity floor as the RTC model
  tm t;
  gmtime_r(&epoch, &t);
  drivers::rtc::DateTimeUtc dt;
  dt.year = t.tm_year + 1900;
  dt.month = t.tm_mon + 1;
  dt.day = t.tm_mday;
  dt.hour = t.tm_hour;
  dt.minute = t.tm_min;
  dt.second = t.tm_sec;
  if (!drivers::rtc::isSane(dt) || !drivers::rtc::write(dt)) return false;
  setSystemClock(epoch);
  st.rtcValid = true;
  st.timeSet = true;
  return true;
}

namespace {
// Parse and apply one input line. Returns true only when the clock was set.
bool applyTimeLine(const char* line) {
  if (strncmp(line, "TIME", 4) != 0) return false;
  const char* arg = line + 4;
  while (*arg == ' ' || *arg == '\t' || *arg == '=' || *arg == ':') ++arg;
  if (*arg == 0) {
    Serial.println("scribr: TIME command needs an argument (unix epoch or ISO-8601 UTC)");
    return false;
  }

  time_t epoch = 0;
  if (strchr(arg, '-') || strchr(arg, 'T')) {
    drivers::rtc::DateTimeUtc dt;
    if (!drivers::rtc::fromIso8601(arg, dt) || !drivers::rtc::toEpoch(dt, epoch)) {
      Serial.printf("scribr: TIME parse failed for '%s'\n", arg);
      return false;
    }
  } else {
    char* end = nullptr;
    long long v = strtoll(arg, &end, 10);
    if (end == arg) {
      Serial.printf("scribr: TIME parse failed for '%s'\n", arg);
      return false;
    }
    epoch = (time_t)v;
  }

  if (!setFromEpoch(epoch)) {
    Serial.printf("scribr: TIME rejected (out of range or RTC write failed) epoch=%lld\n", (long long)epoch);
    return false;
  }
  Serial.printf("scribr: RTC set from serial; epoch=%lld UTC\n", (long long)epoch);
  return true;
}
}  // namespace

bool pollSerialTimeSet() {
  static char buf[40];
  static size_t len = 0;
  bool didSet = false;
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[len] = 0;
      if (applyTimeLine(buf)) didSet = true;
      len = 0;
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = (char)c;
    } else {
      len = 0;  // overrun on a junk line; drop it rather than wedge
    }
  }
  return didSet;
}

}  // namespace services::clock
