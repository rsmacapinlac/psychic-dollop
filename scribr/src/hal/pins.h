#pragma once
// scribr board pin map — Waveshare ESP32-S3 1.54" e-Paper (ESP32-S3-PICO-1).
// Full GPIO map: docs/requirements/hardware-pinout.md.
// As we add features (buttons, audio, SD, RTC), extend this file.

// ── E-paper panel (SPI) ─────────────────────────────────────────────
#define EPD_DC_PIN    10
#define EPD_CS_PIN    11
#define EPD_SCK_PIN   12
#define EPD_MOSI_PIN  13
#define EPD_RST_PIN    9
#define EPD_BUSY_PIN   8

// ── Power rails (active-low enables on this board) ──────────────────
#define EPD_PWR_PIN    6
#define AUDIO_PWR_PIN 42
#define VBAT_PWR_PIN  17

// ── Buttons (active-low, internal pull-ups) ─────────────────────────
#define BTN_BOOT_PIN   0   // labeled BOOT
#define BTN_PWR_PIN   18   // labeled PWR
