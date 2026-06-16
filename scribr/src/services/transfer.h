#pragma once

#include <stddef.h>
#include <stdint.h>

// v1 ships a seam for future BLE sync/config, not a transport implementation.
// Storage and app code should depend on this shape only when sync is added.
namespace services::transfer {

struct NoteSummary {
  uint16_t id = 0;
  char wavPath[32] = {0};
  char metaPath[32] = {0};
  char createdUtc[21] = {0};  // YYYY-MM-DDTHH:MM:SSZ, empty when unknown
  uint32_t durationSec = 0;
  uint32_t byteSize = 0;
};

class NoteReader {
 public:
  virtual ~NoteReader() = default;
  virtual size_t read(uint8_t* dst, size_t maxLen) = 0;
  virtual bool seek(uint32_t offset) = 0;
  virtual uint32_t size() const = 0;
};

class Interface {
 public:
  virtual ~Interface() = default;

  virtual uint16_t noteCount() = 0;
  virtual bool noteAt(uint16_t index, NoteSummary& out) = 0;
  virtual NoteReader* openNote(uint16_t id) = 0;

  // Future companion-app config path. Implementations may directly read/update
  // /scribr.cfg or call a config API that persists back to that file.
  virtual bool readConfig(char* dst, size_t maxLen, size_t& written) = 0;
  virtual bool writeConfigAtomically(const char* text, size_t len) = 0;
};

}  // namespace services::transfer
