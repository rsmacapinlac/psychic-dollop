#pragma once

#include <stdint.h>
#include <stddef.h>
#include <time.h>

namespace services::storage {

// On-disk layout (filesystem-as-state, mirroring the earshot project, ADR 0004):
// one recording == one directory under /recordings, named with the UTC capture
// time as %Y%m%dT%H%M%S (e.g. 20260618T091500). When the clock is not yet set,
// an "unset-NNN" fallback name is used so a note can still be captured. Each
// directory holds:
//   session.wav   mono 16 kHz/16-bit PCM, 44-byte header backfilled at stop
//   session.meta  sidecar holding duration_sec=<seconds>
// The directory name is the authoritative creation timestamp; the sidecar is the
// authoritative duration. No index/counter file is kept — the scan is the truth.
struct NoteSummary {
  char dir[20] = {0};        // directory name, e.g. "20260618T091500" or "unset-001"
  char wavPath[48] = {0};    // "/recordings/<dir>/session.wav"
  char metaPath[48] = {0};   // "/recordings/<dir>/session.meta"
  uint32_t durationSec = 0;
  uint32_t byteSize = 0;
  time_t createdEpoch = 0;   // UTC epoch parsed from dir name; valid when hasTimestamp
  bool hasTimestamp = false;
};

void begin();
bool available();
void rescan();
size_t noteCount();
bool noteAt(size_t index, NoteSummary& out);  // sorted newest first

// Create the session directory for a new recording and return its paths. When
// hasTime is true the directory is named from createdUtc (%Y%m%dT%H%M%S, UTC);
// otherwise an "unset-NNN" fallback is used so capture still works before the
// clock is configured.
bool newNote(bool hasTime, time_t createdUtc,
             char* dirOut, size_t dirLen,
             char* wavOut, size_t wavLen,
             char* metaOut, size_t metaLen);

// Write the duration sidecar for an existing recording directory.
bool writeMetadata(const char* dir, uint32_t durationSec);

// Remove a recording directory and its contents (session.wav + session.meta).
bool deleteNote(const char* dir);

}  // namespace services::storage
