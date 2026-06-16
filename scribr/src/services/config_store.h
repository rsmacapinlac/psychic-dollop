#pragma once

#include <stddef.h>
#include <stdint.h>

namespace services::config {

struct BootConfig {
  int16_t localOffsetMin = 0;
  bool hasRtcSetUtc = false;
  char rtcSetUtc[21] = {0};  // YYYY-MM-DDTHH:MM:SSZ
};

// Parse ASCII /scribr.cfg text. Unknown keys and invalid values are ignored
// independently. Missing/invalid local_offset_min falls back to 0.
bool parseBootConfigText(const char* text, BootConfig& out);

// Load /scribr.cfg from the SD-card root. Missing/unreadable config is not
// fatal and returns false with safe defaults in out.
bool loadBootConfig(BootConfig& out);

// After a successful RTC bootstrap, mark the exact rtc_set_utc value as
// consumed so stale time is not reapplied on the next boot. The preferred path
// rewrites /scribr.cfg, preserving comments/unknown keys and commenting matching
// active rtc_set_utc lines as "# applied rtc_set_utc=...".
bool markRtcSetApplied(const char* rtcSetUtc);

bool isValidLocalOffset(long minutes);
bool isValidUtcTimestamp(const char* value);

}  // namespace services::config
