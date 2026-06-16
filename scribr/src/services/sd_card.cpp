#include "sd_card.h"

#include <Arduino.h>
#include <SD_MMC.h>

#include "../hal/pins.h"

namespace services::sdcard {
namespace {
bool didBegin = false;
bool isMounted = false;
uint32_t lastAttemptMs = 0;

bool mountOnce() {
  lastAttemptMs = millis();
  SD_MMC.end();
  SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN);
  isMounted = SD_MMC.begin("/sdcard", true /* 1-bit mode */, false /* no format */);
  if (!isMounted) {
    Serial.println("scribr: SD_MMC mount failed");
    return false;
  }
  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("scribr: no SD card");
    SD_MMC.end();
    isMounted = false;
    return false;
  }
  if (!SD_MMC.exists("/notes")) SD_MMC.mkdir("/notes");
  Serial.println("scribr: SD_MMC mounted");
  return true;
}
}  // namespace

void begin() {
  if (didBegin && isMounted) return;
  didBegin = true;
  mountOnce();
}

bool ensureMounted() {
  if (isMounted && SD_MMC.cardType() != CARD_NONE) return true;
  isMounted = false;
  // Avoid hammering the bus from the main tick loop, but allow quick retries
  // after a user reseats the card and presses a button.
  if (millis() - lastAttemptMs < 1000UL) return false;
  return mountOnce();
}

bool mounted() { return ensureMounted(); }
fs::FS& fs() { return SD_MMC; }
uint64_t totalBytes() { return isMounted ? SD_MMC.totalBytes() : 0; }
uint64_t usedBytes() { return isMounted ? SD_MMC.usedBytes() : 0; }
uint64_t freeBytes() {
  if (!isMounted) return 0;
  const uint64_t total = totalBytes();
  const uint64_t used = usedBytes();
  return total > used ? total - used : 0;
}
float freeGB() { return freeBytes() / (1024.0f * 1024.0f * 1024.0f); }

}  // namespace services::sdcard
