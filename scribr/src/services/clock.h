#pragma once

#include <time.h>
#include "config_store.h"

namespace services::clock {

struct Status {
  bool rtcValid = false;
  bool timeSet = false;
  bool bootstrapApplied = false;
  int16_t localOffsetMin = 0;
};

void begin(const services::config::BootConfig& cfg);
Status status();
bool nowUtc(time_t& out);
void formatLocal(time_t utc, char* out, size_t outLen);  // YYYY-MM-DD HH:MM

// Set the RTC + system clock from a UTC epoch (seconds). Validates range/sanity,
// writes the PCF85063, and updates status to timeSet. Returns false on a bad
// value or RTC write failure.
bool setFromEpoch(time_t epoch);

// Non-blocking serial time-set: drains pending Serial input and, on a complete
// line of the form "TIME <unix-epoch>" or "TIME <YYYY-MM-DDTHH:MM:SSZ>", sets the
// clock via setFromEpoch(). Call once per tick. Returns true only on the tick a
// set succeeds, so the caller can clear the TIME NOT SET screen and redraw.
bool pollSerialTimeSet();

}  // namespace services::clock
