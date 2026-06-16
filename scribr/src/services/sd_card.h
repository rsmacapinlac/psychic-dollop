#pragma once

#include <FS.h>

namespace services::sdcard {

void begin();
// Retry a mount attempt if the previous one failed or the card was inserted
// after boot. Safe to call from user-triggered actions.
bool ensureMounted();
bool mounted();

// Force the next mount check to re-probe the hardware. Call after an I/O failure
// that suggests the card was removed or hot-swapped; cardType() is latched at
// mount time and cannot detect that on its own.
void invalidate();

// Real round-trip write test (create + delete a marker file). Detects a
// swapped/removed card that the cached cardType() reports as still present.
bool probeWritable();
fs::FS& fs();
uint64_t totalBytes();
uint64_t usedBytes();
uint64_t freeBytes();
float freeGB();

}  // namespace services::sdcard
