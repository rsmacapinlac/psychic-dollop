// scribr — custom firmware for the Pala Note (ESP32-S3-PICO-1)
//
// Entry point. Arduino requires the .ino filename to match the folder name.
// Layered source lives under scribr/src/:
//   hal/      board pin map + power rails
//   drivers/  raw e-paper panel driver (reused from the reference firmware)
//   app/      display service, screens, and the UX state machine
//
// This is a UX prototype: recordings are fake and timers are simulated so the
// screen flow can be evaluated on real hardware before audio/SD are wired in.

#include "src/app/app.h"
#include "src/app/display.h"

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("scribr: boot");

  display::begin();   // power rails + panel init
  app::begin();       // buttons + first screen
  Serial.println("scribr: ready");
}

void loop() {
  app::tick();        // service buttons, timers, redraws
  delay(1);           // 1ms button polling; yields to idle/watchdog
}
