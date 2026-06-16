#pragma once

#include <stdint.h>
#include <stddef.h>
#include <time.h>

namespace services::storage {

struct NoteSummary {
  uint16_t id = 0;
  char wavPath[32] = {0};
  char metaPath[32] = {0};
  char createdUtc[21] = {0};
  uint32_t durationSec = 0;
  uint32_t byteSize = 0;
  time_t createdEpoch = 0;  // UTC epoch, valid only when hasTimestamp=true
  bool hasTimestamp = false;
};

void begin();
bool available();
void rescan();
size_t noteCount();
bool noteAt(size_t index, NoteSummary& out);  // sorted newest first
uint16_t nextNoteId();
bool deleteNote(uint16_t id);
bool pathsForId(uint16_t id, char* wavOut, size_t wavLen, char* metaOut, size_t metaLen);
bool writeMetadata(uint16_t id, uint32_t durationSec, bool hasCreatedUtc, time_t createdUtc);

}  // namespace services::storage
