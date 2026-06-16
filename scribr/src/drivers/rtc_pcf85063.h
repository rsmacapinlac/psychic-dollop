#pragma once

#include <stdint.h>
#include <time.h>

namespace drivers::rtc {

struct DateTimeUtc {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

void begin();
bool read(DateTimeUtc& out, bool& oscillatorStopped);
bool write(const DateTimeUtc& value);
bool isSane(const DateTimeUtc& value);
bool toEpoch(const DateTimeUtc& value, time_t& out);
bool fromIso8601(const char* iso, DateTimeUtc& out);
void toIso8601(const DateTimeUtc& value, char* out, size_t outLen);

}  // namespace drivers::rtc
