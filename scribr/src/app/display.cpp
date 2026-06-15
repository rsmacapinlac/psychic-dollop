#include "display.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../hal/board_power_bsp.h"
#include "../hal/pins.h"
#include "../drivers/epaper_driver_bsp.h"  // pulls in SPI2_HOST via driver/spi_master.h

namespace {
// Board power rails (EPD, audio, VBAT). Constructed at startup like the
// reference firmware's global `board` — GPIO config runs before setup().
board_power_bsp_t      power(EPD_PWR_PIN, AUDIO_PWR_PIN, VBAT_PWR_PIN);
epaper_driver_display* epd = nullptr;

// The 1-bit drawing surface. Same dimensions/stride as the panel buffer.
GFXcanvas1 cv(display::WIDTH, display::HEIGHT);

// ── Background refresh ───────────────────────────────────────────────
// The panel refresh busy-waits ~300-500ms. If we did it inline it would
// starve the main loop's button polling. Instead a task pinned to the other
// core performs the SPI/refresh while loop() keeps servicing buttons.
TaskHandle_t    refreshTask = nullptr;
volatile bool   refreshBusy = false;
volatile bool   wantFull    = false;

// Copy the canvas into the panel framebuffer, inverting polarity:
// canvas bit 1 = ink -> panel bit 0 = black; canvas 0 -> panel 1 = white.
// Runs on the main core, only while no refresh is in flight (see busy()).
void blit() {
  const uint8_t* src = cv.getBuffer();
  uint8_t*       dst = epd->getBuffer();
  const int      n   = (display::WIDTH * display::HEIGHT) / 8;
  for (int i = 0; i < n; i++) dst[i] = ~src[i];
}

void refreshTaskFn(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // wait for a refresh request
    if (wantFull) {
      epd->EPD_Init();                 // back to full mode
      epd->EPD_DisplayPartBaseImage(); // full flash, set new base
      epd->EPD_Init_Partial();         // resume partial mode
    } else {
      epd->EPD_DisplayPart();          // fast, no flash
    }
    refreshBusy = false;
  }
}

void requestRefresh(bool full) {
  if (!epd || refreshBusy) return;
  blit();                       // safe: no refresh in flight
  wantFull    = full;
  refreshBusy = true;
  xTaskNotifyGive(refreshTask); // hand off to the 2nd core
}
}  // namespace

namespace display {

void begin() {
  power.VBAT_POWER_ON();   // latch the board's own power on
  power.POWEER_EPD_ON();   // enable the e-paper rail
  delay(200);              // let the panel settle before talking to it

  custom_lcd_spi_t cfg = {};
  cfg.cs         = EPD_CS_PIN;
  cfg.dc         = EPD_DC_PIN;
  cfg.rst        = EPD_RST_PIN;
  cfg.busy       = EPD_BUSY_PIN;
  cfg.mosi       = EPD_MOSI_PIN;
  cfg.scl        = EPD_SCK_PIN;
  cfg.spi_host   = SPI2_HOST;
  cfg.buffer_len = (WIDTH * HEIGHT) / 8;

  epd = new epaper_driver_display(WIDTH, HEIGHT, cfg);
  epd->EPD_Init();                 // full mode
  epd->EPD_Clear();                // buffer -> white
  epd->EPD_DisplayPartBaseImage(); // push white base (full flash)
  epd->EPD_Init_Partial();         // switch to partial mode

  // Refresh runs on core 0; the Arduino loop runs on core 1.
  xTaskCreatePinnedToCore(refreshTaskFn, "epd_refresh", 4096, nullptr, 1, &refreshTask, 0);
}

GFXcanvas1& canvas() { return cv; }
bool        busy()   { return refreshBusy; }
void        flush()      { requestRefresh(false); }
void        flushFull()  { requestRefresh(true); }

}  // namespace display
