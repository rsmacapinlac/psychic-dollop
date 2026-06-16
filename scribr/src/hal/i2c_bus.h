#pragma once

#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace hal::i2c {

constexpr uint32_t DEFAULT_FREQ_HZ = 400000;

void begin(uint32_t freqHz = DEFAULT_FREQ_HZ);
TwoWire& bus();
SemaphoreHandle_t mutex();

class Lock {
 public:
  explicit Lock(TickType_t timeout = pdMS_TO_TICKS(1000));
  ~Lock();
  bool acquired() const { return acquired_; }

 private:
  bool acquired_ = false;
};

}  // namespace hal::i2c
