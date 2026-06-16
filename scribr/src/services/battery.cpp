#include "battery.h"

#include <Arduino.h>

#include "../hal/pins.h"

namespace services::battery {
namespace {
constexpr uint32_t CHECK_INTERVAL_MS = 30000;
Reading cached;
uint32_t lastCheckMs = 0;
bool lowLatched = false;

int snap5(float pct) {
  int snapped = (int)((pct + 2.5f) / 5.0f) * 5;
  if (snapped < 0) snapped = 0;
  if (snapped > 100) snapped = 100;
  return snapped;
}

float lerp(float v, float v0, float p0, float v1, float p1) {
  return p0 + (v - v0) * (p1 - p0) / (v1 - v0);
}

int percentForVoltage(float v) {
  // Calibrated on target hardware: a fully charged/USB-powered pack reads
  // about 4.13V through the board ADC path, so treat 4.12V+ as full instead of
  // the ideal cell voltage of 4.20V.
  if (v >= 4.12f) return 100;
  if (v <= 3.20f) return 0;
  if (v < 3.40f) return snap5(lerp(v, 3.20f, 0, 3.40f, 25));
  if (v < 3.70f) return snap5(lerp(v, 3.40f, 25, 3.70f, 50));
  if (v < 3.90f) return snap5(lerp(v, 3.70f, 50, 3.90f, 75));
  return snap5(lerp(v, 3.90f, 75, 4.12f, 100));
}
}  // namespace

void begin() {
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  cached = readNow();
  lastCheckMs = millis();
}

Reading readNow() {
  uint32_t sumMv = 0;
  for (int i = 0; i < 16; ++i) {
    sumMv += analogReadMilliVolts(BATTERY_ADC_PIN);
    delay(2);
  }
  const float pinV = (sumMv / 16.0f) / 1000.0f;
  const float packV = pinV * 2.0f;

  Reading r;
  r.voltage = packV;
  r.valid = packV > 0.1f;
  r.percent = r.valid ? percentForVoltage(packV) : -1;

  if (r.valid) {
    if (!lowLatched && r.percent <= 15) lowLatched = true;
    else if (lowLatched && r.percent >= 20) lowLatched = false;
  }
  r.low = lowLatched;
  cached = r;
  return r;
}

Reading last() { return cached; }

void tick() {
  if (millis() - lastCheckMs >= CHECK_INTERVAL_MS) {
    lastCheckMs = millis();
    readNow();
  }
}

}  // namespace services::battery
