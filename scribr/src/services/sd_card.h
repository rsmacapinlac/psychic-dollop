#pragma once

#include <FS.h>

namespace services::sdcard {

void begin();
// Retry a mount attempt if the previous one failed or the card was inserted
// after boot. Safe to call from user-triggered actions.
bool ensureMounted();
bool mounted();
fs::FS& fs();
uint64_t totalBytes();
uint64_t usedBytes();
uint64_t freeBytes();
float freeGB();

}  // namespace services::sdcard
