#include "sleep_service.h"

#include <Arduino.h>
#include <esp_sleep.h>

#include "../hal/pins.h"
#include "../hal/power.h"
#include "audio_service.h"

namespace services::sleep {

void prepareForSleep() {
  audio::stopPlayback();
  hal::power::disableAudioRail();
  hal::power::holdVbatLatch();
}

void enterLightSleepUntilButton() {
  prepareForSleep();
  const uint64_t mask = (1ULL << BTN_BOOT_PIN) | (1ULL << BTN_PWR_PIN);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
  Serial.println("scribr: entering light sleep");
  Serial.flush();
  esp_light_sleep_start();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
  Serial.println("scribr: woke from light sleep");
}

void wakeToIdle() {
  hal::power::holdVbatLatch();
}

}  // namespace services::sleep
