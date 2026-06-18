#include "storage.h"

#include <Arduino.h>
#include <FS.h>
#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "sd_card.h"

namespace services::storage {
namespace {
constexpr char kRoot[] = "/recordings";

std::vector<NoteSummary> notes;
unsigned highestUnset = 0;  // highest N seen in "unset-NNN" dirs, for fallback naming

void buildPaths(const char* dir, char* wavOut, size_t wavLen, char* metaOut, size_t metaLen) {
  if (wavOut && wavLen) snprintf(wavOut, wavLen, "%s/%s/session.wav", kRoot, dir);
  if (metaOut && metaLen) snprintf(metaOut, metaLen, "%s/%s/session.meta", kRoot, dir);
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

// Parse a directory name shaped like %Y%m%dT%H%M%S (e.g. "20260618T091500", 15
// chars) into a UTC epoch. The directory name is the authoritative timestamp.
bool parseStampEpoch(const char* name, time_t& out) {
  if (!name || strlen(name) != 15 || name[8] != 'T') return false;
  const int year = fourDigits(name);
  const int month = twoDigits(name + 4);
  const int day = twoDigits(name + 6);
  const int hour = twoDigits(name + 9);
  const int minute = twoDigits(name + 11);
  const int second = twoDigits(name + 13);
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
    if (line.startsWith("duration_sec=")) {
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
  if (!sdcard::fs().exists(kRoot)) sdcard::fs().mkdir(kRoot);
  return true;
}

void rescan() {
  notes.clear();
  highestUnset = 0;
  if (!available()) return;

  File root = sdcard::fs().open(kRoot);
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }

  // Collect directory names first, then process them. Opening session.wav while
  // the directory handle is still iterating is fragile on SD_MMC; gather, close,
  // then read.
  std::vector<String> dirs;
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    if (e.isDirectory()) {
      String path = e.path();
      const char* base = strrchr(path.c_str(), '/');
      dirs.emplace_back(base ? base + 1 : path.c_str());
    }
    e.close();
  }
  root.close();

  for (const String& name : dirs) {
    unsigned u = 0;
    if (sscanf(name.c_str(), "unset-%u", &u) == 1 && u > highestUnset) highestUnset = u;

    NoteSummary n;
    strncpy(n.dir, name.c_str(), sizeof(n.dir) - 1);
    buildPaths(n.dir, n.wavPath, sizeof(n.wavPath), n.metaPath, sizeof(n.metaPath));

    // A directory counts as a recording only once session.wav holds real audio.
    // A partial/cancelled capture (no wav, or header-only) is ignored by the scan.
    File w = sdcard::fs().open(n.wavPath, FILE_READ);
    if (!w) continue;
    const uint32_t size = (uint32_t)w.size();
    w.close();
    if (size <= 44) continue;

    n.byteSize = size;
    time_t epoch = 0;
    if (parseStampEpoch(n.dir, epoch)) {
      n.createdEpoch = epoch;
      n.hasTimestamp = true;
    }
    parseMeta(n);
    notes.push_back(n);
  }

  std::sort(notes.begin(), notes.end(), [](const NoteSummary& a, const NoteSummary& b) {
    if (a.hasTimestamp != b.hasTimestamp) return a.hasTimestamp;
    if (a.hasTimestamp && b.hasTimestamp) return a.createdEpoch > b.createdEpoch;
    return strcmp(a.dir, b.dir) > 0;
  });
}

size_t noteCount() { return notes.size(); }

bool noteAt(size_t index, NoteSummary& out) {
  if (index >= notes.size()) return false;
  out = notes[index];
  return true;
}

bool newNote(bool hasTime, time_t createdUtc,
             char* dirOut, size_t dirLen,
             char* wavOut, size_t wavLen,
             char* metaOut, size_t metaLen) {
  if (!available()) return false;

  char dir[20];
  if (hasTime) {
    tm t;
    gmtime_r(&createdUtc, &t);
    snprintf(dir, sizeof(dir), "%04d%02d%02dT%02d%02d%02d",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    snprintf(dir, sizeof(dir), "unset-%03u", highestUnset + 1);
    // Reserve this fallback index now so a second capture before the next rescan
    // does not collide on the same name.
    ++highestUnset;
  }

  char full[40];
  snprintf(full, sizeof(full), "%s/%s", kRoot, dir);
  if (!sdcard::fs().exists(full) && !sdcard::fs().mkdir(full)) return false;

  if (dirOut && dirLen) {
    strncpy(dirOut, dir, dirLen - 1);
    dirOut[dirLen - 1] = 0;
  }
  buildPaths(dir, wavOut, wavLen, metaOut, metaLen);
  return true;
}

bool writeMetadata(const char* dir, uint32_t durationSec) {
  if (!available() || !dir || !dir[0]) return false;
  char meta[48];
  buildPaths(dir, nullptr, 0, meta, sizeof(meta));
  File f = sdcard::fs().open(meta, FILE_WRITE);
  if (!f) return false;
  f.print("duration_sec=");
  f.println(durationSec);
  f.close();
  return true;
}

bool deleteNote(const char* dir) {
  if (!available() || !dir || !dir[0]) return false;
  char wav[48], meta[48], full[40];
  buildPaths(dir, wav, sizeof(wav), meta, sizeof(meta));
  snprintf(full, sizeof(full), "%s/%s", kRoot, dir);
  bool ok = true;
  if (sdcard::fs().exists(wav)) ok = sdcard::fs().remove(wav) && ok;
  if (sdcard::fs().exists(meta)) ok = sdcard::fs().remove(meta) && ok;
  if (sdcard::fs().exists(full)) ok = sdcard::fs().rmdir(full) && ok;
  rescan();
  return ok;
}

}  // namespace services::storage
