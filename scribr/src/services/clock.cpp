#include "clock.h"

#include <Arduino.h>
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

}  // namespace services::clock
