#include "rtc_pcf85063.h"

#include <Arduino.h>
#include <string.h>

#include "../hal/i2c_bus.h"

namespace drivers::rtc {
namespace {
constexpr uint8_t ADDR = 0x51;
constexpr uint8_t REG_SECONDS = 0x04;

uint8_t bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
uint8_t dec(uint8_t b) { return ((b >> 4) * 10) + (b & 0x0F); }

int twoDigits(const char* s) {
  if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1])) return -1;
  return (s[0] - '0') * 10 + (s[1] - '0');
}

int fourDigits(const char* s) {
  for (int i = 0; i < 4; ++i) if (!isdigit((unsigned char)s[i])) return -1;
  return (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0');
}
}  // namespace

void begin() { hal::i2c::begin(); }

bool read(DateTimeUtc& out, bool& oscillatorStopped) {
  begin();
  hal::i2c::Lock lock;
  if (!lock.acquired()) return false;

  TwoWire& w = hal::i2c::bus();
  w.beginTransmission(ADDR);
  w.write(REG_SECONDS);
  if (w.endTransmission(false) != 0) return false;
  if (w.requestFrom((int)ADDR, 7) != 7) return false;

  uint8_t raw[7];
  for (uint8_t& b : raw) b = w.read();
  oscillatorStopped = raw[0] & 0x80;

  out.second = dec(raw[0] & 0x7F);
  out.minute = dec(raw[1] & 0x7F);
  out.hour = dec(raw[2] & 0x3F);
  out.day = dec(raw[3] & 0x3F);
  out.month = dec(raw[5] & 0x1F);
  out.year = 2000 + dec(raw[6]);
  return true;
}

bool write(const DateTimeUtc& value) {
  if (!isSane(value)) return false;
  begin();
  hal::i2c::Lock lock;
  if (!lock.acquired()) return false;

  TwoWire& w = hal::i2c::bus();
  w.beginTransmission(ADDR);
  w.write(REG_SECONDS);
  w.write(bcd(value.second));
  w.write(bcd(value.minute));
  w.write(bcd(value.hour));
  w.write(bcd(value.day));
  w.write((uint8_t)0);  // weekday unused
  w.write(bcd(value.month));
  w.write(bcd(value.year - 2000));
  return w.endTransmission() == 0;
}

bool isSane(const DateTimeUtc& v) {
  if (v.year < 2024 || v.year > 2099) return false;
  if (v.month < 1 || v.month > 12) return false;
  if (v.day < 1 || v.day > 31) return false;
  if (v.hour < 0 || v.hour > 23) return false;
  if (v.minute < 0 || v.minute > 59) return false;
  if (v.second < 0 || v.second > 59) return false;
  return true;
}

bool toEpoch(const DateTimeUtc& value, time_t& out) {
  if (!isSane(value)) return false;
  setenv("TZ", "UTC0", 1);
  tzset();
  tm t = {};
  t.tm_year = value.year - 1900;
  t.tm_mon = value.month - 1;
  t.tm_mday = value.day;
  t.tm_hour = value.hour;
  t.tm_min = value.minute;
  t.tm_sec = value.second;
  out = mktime(&t);
  return out >= 1700000000;
}

bool fromIso8601(const char* iso, DateTimeUtc& out) {
  if (!iso || strlen(iso) != 20) return false;
  if (iso[4] != '-' || iso[7] != '-' || iso[10] != 'T' || iso[13] != ':' || iso[16] != ':' || iso[19] != 'Z') return false;
  out.year = fourDigits(iso);
  out.month = twoDigits(iso + 5);
  out.day = twoDigits(iso + 8);
  out.hour = twoDigits(iso + 11);
  out.minute = twoDigits(iso + 14);
  out.second = twoDigits(iso + 17);
  time_t epoch;
  return toEpoch(out, epoch);
}

void toIso8601(const DateTimeUtc& v, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen, "%04d-%02d-%02dT%02d:%02d:%02dZ", v.year, v.month, v.day, v.hour, v.minute, v.second);
}

}  // namespace drivers::rtc
