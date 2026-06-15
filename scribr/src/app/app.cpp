#include "app.h"

#include <Arduino.h>
#include <OneButton.h>
#include <stdio.h>
#include <string.h>

#include "../hal/pins.h"
#include "display.h"
#include "model.h"
#include "screens.h"

namespace {

app::Model model;

// Two buttons, active-low with internal pull-ups (pin, activeLow, pullupActive).
OneButton btnBoot(BTN_BOOT_PIN, true, true);
OneButton btnPwr(BTN_PWR_PIN, true, true);

// Redraw bookkeeping. needFull is sticky for the frame: any full request wins.
bool     needRender = true;
bool     needFull   = true;
uint32_t recStartMs  = 0;
uint32_t playStartMs = 0;

void requestRender(bool full) {
  needRender = true;
  if (full) needFull = true;
}

void go(app::Screen s, bool full) {
  model.screen = s;
  requestRender(full);
}

// ── fake data + mutations (prototype) ───────────────────────────────
void seedRecordings() {
  const struct { const char* date; int dur; } seed[] = {
    {"2026-06-14 09:32", 14}, {"2026-06-14 08:01", 128}, {"2026-06-13 18:05", 95},
    {"2026-06-13 08:47", 312}, {"2026-06-12 22:10", 53}, {"2026-06-11 21:19", 47},
  };
  model.recCount = sizeof(seed) / sizeof(seed[0]);
  for (int i = 0; i < model.recCount; i++) {
    strncpy(model.recs[i].date, seed[i].date, sizeof(model.recs[i].date) - 1);
    model.recs[i].dur = seed[i].dur;
  }
}

void addRecording(int dur) {
  if (model.recCount >= app::MAX_RECS) model.recCount = app::MAX_RECS - 1;  // drop oldest
  for (int i = model.recCount; i > 0; i--) model.recs[i] = model.recs[i - 1];
  snprintf(model.recs[0].date, sizeof(model.recs[0].date), "2026-06-14 %02d:%02d",
           9 + (model.recCount % 12), (model.recCount * 7) % 60);
  model.recs[0].dur = dur;
  model.recCount++;
}

void deleteCurrent() {
  if (model.recCount == 0) return;
  for (int i = model.listIndex; i < model.recCount - 1; i++) model.recs[i] = model.recs[i + 1];
  model.recCount--;
  if (model.listIndex >= model.recCount) model.listIndex = model.recCount > 0 ? model.recCount - 1 : 0;
}

// ── button callbacks (dispatch on current screen) ───────────────────
void onBootClick() {
  switch (model.screen) {
    case app::Screen::REC: go(app::Screen::IDLE, true); break;  // cancel: discard, don't save
    case app::Screen::LIST:
      if (model.recCount > 0) { model.listIndex = (model.listIndex + 1) % model.recCount; requestRender(true); }
      break;
    case app::Screen::DELETE_CONFIRM: go(app::Screen::LIST, true); break;  // cancel
    default: break;
  }
}
void onBootDouble() {
  if (model.screen == app::Screen::IDLE) { model.listIndex = 0; go(app::Screen::LIST, true); }
}
void onBootLong() {
  if (model.screen == app::Screen::LIST) go(app::Screen::IDLE, true);  // exit
}

void onPwrClick() {
  switch (model.screen) {
    case app::Screen::REC:
      addRecording(model.recElapsed); go(app::Screen::IDLE, true);  // save
      break;
    case app::Screen::LIST:
      if (model.recCount > 0) { model.playElapsed = 0; playStartMs = millis(); go(app::Screen::PLAY, true); }
      break;
    case app::Screen::PLAY: go(app::Screen::IDLE, true); break;  // stop
    case app::Screen::DELETE_CONFIRM: deleteCurrent(); go(app::Screen::LIST, true); break;  // confirm
    default: break;
  }
}
void onPwrDouble() {
  if (model.screen == app::Screen::IDLE) { model.recElapsed = 0; recStartMs = millis(); go(app::Screen::REC, true); }
}
void onPwrLong() {
  if (model.screen == app::Screen::LIST && model.recCount > 0) go(app::Screen::DELETE_CONFIRM, true);
}

// ── timers for the live screens ─────────────────────────────────────
void serviceTimers() {
  if (model.screen == app::Screen::REC) {
    int e = (int)((millis() - recStartMs) / 1000);
    if (e != model.recElapsed) { model.recElapsed = e; requestRender(false); }  // partial tick
  } else if (model.screen == app::Screen::PLAY) {
    int dur = model.recs[model.listIndex].dur;
    int e = (int)((millis() - playStartMs) / 1000);
    if (e >= dur) { go(app::Screen::IDLE, true); }                              // auto-finish
    else if (e != model.playElapsed) { model.playElapsed = e; requestRender(false); }
  }
}

}  // namespace

void app::begin() {
  seedRecordings();

  btnBoot.attachClick(onBootClick);
  btnBoot.attachDoubleClick(onBootDouble);
  btnBoot.attachLongPressStart(onBootLong);
  btnPwr.attachClick(onPwrClick);
  btnPwr.attachDoubleClick(onPwrDouble);
  btnPwr.attachLongPressStart(onPwrLong);

  go(app::Screen::IDLE, true);
}

void app::tick() {
  btnBoot.tick();
  btnPwr.tick();
  serviceTimers();

  // Dispatch a redraw only when the previous refresh has finished (it runs on
  // the other core). Until then buttons keep being serviced above.
  if (needRender && !display::busy()) {
    screens::render(model);
    if (needFull) display::flushFull(); else display::flush();
    needRender = false;
    needFull = false;
  }
}
