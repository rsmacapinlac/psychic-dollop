#include "storage.h"

#include <Arduino.h>
#include <FS.h>
#include <algorithm>
#include <ctype.h>
#include <time.h>
#include <vector>

#include "sd_card.h"

namespace services::storage {
namespace {
std::vector<NoteSummary> notes;
uint16_t highestId = 0;

bool parseId(const char* name, uint16_t& id) {
  const char* base = strrchr(name, '/');
  base = base ? base + 1 : name;
  if (strncmp(base, "note_", 5) != 0) return false;
  if (!isdigit((unsigned char)base[5]) || !isdigit((unsigned char)base[6]) || !isdigit((unsigned char)base[7])) return false;
  if (strcmp(base + 8, ".wav") != 0) return false;
  id = (base[5] - '0') * 100 + (base[6] - '0') * 10 + (base[7] - '0');
  return id > 0;
}

void makePaths(uint16_t id, char* wavOut, size_t wavLen, char* metaOut, size_t metaLen) {
  if (wavOut && wavLen) snprintf(wavOut, wavLen, "/notes/note_%03u.wav", id);
  if (metaOut && metaLen) snprintf(metaOut, metaLen, "/notes/note_%03u.meta", id);
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

// UTC civil date -> Unix epoch. Avoids relying on process timezone or non-portable timegm().
int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097LL + (int64_t)doe - 719468LL;
}

bool parseIsoUtcEpoch(const char* value, time_t& out) {
  if (!value || strlen(value) != 20) return false;
  if (value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
      value[13] != ':' || value[16] != ':' || value[19] != 'Z') return false;
  const int year = fourDigits(value);
  const int month = twoDigits(value + 5);
  const int day = twoDigits(value + 8);
  const int hour = twoDigits(value + 11);
  const int minute = twoDigits(value + 14);
  const int second = twoDigits(value + 17);
  if (year < 2024 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
  out = (time_t)(daysFromCivil(year, (unsigned)month, (unsigned)day) * 86400LL + hour * 3600 + minute * 60 + second);
  return out >= 1700000000;
}

void parseMeta(NoteSummary& n) {
  File f = sdcard::fs().open(n.metaPath, FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.startsWith("created_utc=")) {
      String v = line.substring(strlen("created_utc="));
      time_t epoch = 0;
      if (v.length() == 20 && parseIsoUtcEpoch(v.c_str(), epoch)) {
        strncpy(n.createdUtc, v.c_str(), sizeof(n.createdUtc) - 1);
        n.createdEpoch = epoch;
        n.hasTimestamp = true;
      }
    } else if (line.startsWith("duration_sec=")) {
      n.durationSec = (uint32_t)line.substring(strlen("duration_sec=")).toInt();
    } else if (line.startsWith("duration=")) {
      n.durationSec = (uint32_t)line.substring(strlen("duration=")).toInt();
    }
  }
  f.close();
}
}  // namespace

void begin() {
  sdcard::begin();
  if (sdcard::mounted()) rescan();
}

bool available() {
  if (!sdcard::ensureMounted()) return false;
  if (!sdcard::fs().exists("/notes")) sdcard::fs().mkdir("/notes");
  return true;
}

void rescan() {
  notes.clear();
  highestId = 0;
  if (!available()) return;

  File dir = sdcard::fs().open("/notes");
  if (!dir || !dir.isDirectory()) return;

  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    if (f.isDirectory()) {
      f.close();
      continue;
    }
    uint16_t id = 0;
    String path = f.path();
    const uint32_t size = (uint32_t)f.size();
    f.close();
    if (!parseId(path.c_str(), id)) continue;
    if (id > highestId) highestId = id;
    if (size <= 44) continue;

    NoteSummary n;
    n.id = id;
    n.byteSize = size;
    makePaths(id, n.wavPath, sizeof(n.wavPath), n.metaPath, sizeof(n.metaPath));
    parseMeta(n);
    notes.push_back(n);
  }
  dir.close();

  std::sort(notes.begin(), notes.end(), [](const NoteSummary& a, const NoteSummary& b) {
    if (a.hasTimestamp != b.hasTimestamp) return a.hasTimestamp;
    if (a.hasTimestamp && b.hasTimestamp) return a.createdEpoch > b.createdEpoch;
    return a.id > b.id;
  });
}

size_t noteCount() { return notes.size(); }

bool noteAt(size_t index, NoteSummary& out) {
  if (index >= notes.size()) return false;
  out = notes[index];
  return true;
}

uint16_t nextNoteId() { return highestId + 1; }

bool pathsForId(uint16_t id, char* wavOut, size_t wavLen, char* metaOut, size_t metaLen) {
  if (id == 0 || !available()) return false;
  makePaths(id, wavOut, wavLen, metaOut, metaLen);
  return true;
}

bool writeMetadata(uint16_t id, uint32_t durationSec, bool hasCreatedUtc, time_t createdUtc) {
  if (!available() || id == 0) return false;
  char meta[32];
  makePaths(id, nullptr, 0, meta, sizeof(meta));
  File f = sdcard::fs().open(meta, FILE_WRITE);
  if (!f) return false;
  if (hasCreatedUtc) {
    tm t;
    gmtime_r(&createdUtc, &t);
    char iso[21];
    snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    f.print("created_utc=");
    f.println(iso);
  }
  f.print("duration_sec=");
  f.println(durationSec);
  f.close();
  return true;
}

bool deleteNote(uint16_t id) {
  if (!available()) return false;
  char wav[32], meta[32];
  makePaths(id, wav, sizeof(wav), meta, sizeof(meta));
  bool ok = true;
  if (sdcard::fs().exists(wav)) ok = sdcard::fs().remove(wav) && ok;
  if (sdcard::fs().exists(meta)) ok = sdcard::fs().remove(meta) && ok;
  rescan();
  return ok;
}

}  // namespace services::storage
