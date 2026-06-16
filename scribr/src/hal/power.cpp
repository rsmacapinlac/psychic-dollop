#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>

#include "pins.h"

namespace hal::power {
namespace {
bool initialized = false;

void setLevel(int pin, int level) {
  gpio_set_level((gpio_num_t)pin, level);
}
}  // namespace

void begin() {
  if (initialized) return;

  gpio_config_t gpioConf = {};
  gpioConf.intr_type = GPIO_INTR_DISABLE;
  gpioConf.mode = GPIO_MODE_OUTPUT;
  gpioConf.pin_bit_mask = (1ULL << EPD_PWR_PIN) | (1ULL << AUDIO_PWR_PIN) | (1ULL << VBAT_PWR_PIN);
  gpioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpioConf));

  // Safe defaults: keep the board alive, leave optional peripheral rails off
  // until their subsystem explicitly enables them.
  holdVbatLatch();
  disableEpaperRail();
  disableAudioRail();

  initialized = true;
}

void holdVbatLatch() {
  // GPIO17 is the board power-hold latch: HIGH holds power.
  setLevel(VBAT_PWR_PIN, 1);
}

void releaseVbatLatch() {
  // LOW releases the latch and may power the board off. Do not call except for
  // an intentional shutdown path.
  setLevel(VBAT_PWR_PIN, 0);
}

void enableEpaperRail() {
  // Active-low rail enable.
  setLevel(EPD_PWR_PIN, 0);
}

void disableEpaperRail() {
  setLevel(EPD_PWR_PIN, 1);
}

void enableAudioRail() {
  // Active-low rail enable.
  setLevel(AUDIO_PWR_PIN, 0);
}

void disableAudioRail() {
  setLevel(AUDIO_PWR_PIN, 1);
}

}  // namespace hal::power
