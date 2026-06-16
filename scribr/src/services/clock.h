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

}  // namespace services::clock
