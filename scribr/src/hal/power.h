#pragma once

// Centralized board power controls for the Waveshare ESP32-S3 1.54" e-Paper
// board. Keep these names semantic: the VBAT control is a power-hold latch,
// while EPD/audio are active-low peripheral rail enables.
namespace hal::power {

void begin();

void holdVbatLatch();
void releaseVbatLatch();  // intentional shutdown only

void enableEpaperRail();
void disableEpaperRail();

void enableAudioRail();
void disableAudioRail();

}  // namespace hal::power
