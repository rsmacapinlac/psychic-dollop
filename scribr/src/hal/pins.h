#pragma once
// scribr board pin map — Waveshare ESP32-S3 1.54" e-Paper (ESP32-S3-PICO-1).
// Full GPIO map: docs/reference/hardware-pinout.md.
// As we add features (buttons, audio, SD, RTC), extend this file.

// ── E-paper panel (SPI) ─────────────────────────────────────────────
#define EPD_DC_PIN    10
#define EPD_CS_PIN    11
#define EPD_SCK_PIN   12
#define EPD_MOSI_PIN  13
#define EPD_RST_PIN    9
#define EPD_BUSY_PIN   8

// ── Power controls ─────────────────────────────────────────────────
// EPD/audio are active-low peripheral rail enables. VBAT is the board's
// power-hold latch: HIGH holds power, LOW releases it intentionally.
#define EPD_PWR_PIN    6
#define AUDIO_PWR_PIN 42
#define VBAT_PWR_PIN  17

// ── Buttons (active-low, internal pull-ups) ─────────────────────────
#define BTN_BOOT_PIN   0   // labeled BOOT
#define BTN_PWR_PIN   18   // labeled PWR

// ── Battery sense ──────────────────────────────────────────────────
#define BATTERY_ADC_PIN 4   // ADC1_CH3, 11 dB attenuation, x2 divider

// ── I2C bus ────────────────────────────────────────────────────────
#define I2C_SDA_PIN 47
#define I2C_SCL_PIN 48

// ── I2S / ES8311 audio ─────────────────────────────────────────────
#define I2S_MCLK_PIN 14
#define I2S_BCLK_PIN 15
#define I2S_WS_PIN   38
#define I2S_DOUT_PIN 45   // ESP32 -> codec
#define I2S_DIN_PIN  16   // codec -> ESP32
#define AUDIO_PA_PIN 46

// ── SD card (SD-MMC, 1-bit mode) ───────────────────────────────────
#define SD_CLK_PIN 39
#define SD_CMD_PIN 41
#define SD_D0_PIN  40
