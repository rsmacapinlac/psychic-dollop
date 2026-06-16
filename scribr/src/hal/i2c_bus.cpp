#include "i2c_bus.h"

#include "pins.h"

namespace hal::i2c {
namespace {
bool initialized = false;
SemaphoreHandle_t busMutex = nullptr;
}  // namespace

void begin(uint32_t freqHz) {
  if (!busMutex) busMutex = xSemaphoreCreateMutex();
  if (initialized) return;
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, freqHz);
  initialized = true;
}

TwoWire& bus() {
  begin();
  return Wire;
}

SemaphoreHandle_t mutex() {
  begin();
  return busMutex;
}

Lock::Lock(TickType_t timeout) {
  begin();
  acquired_ = busMutex && xSemaphoreTake(busMutex, timeout) == pdTRUE;
}

Lock::~Lock() {
  if (acquired_ && busMutex) xSemaphoreGive(busMutex);
}

}  // namespace hal::i2c
