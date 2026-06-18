#include "app.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "display.h"
#include "model.h"
#include "screens.h"
#include "../services/audio_service.h"
#include "../services/battery.h"
#include "../services/clock.h"
#include "../services/config_store.h"
#include "../services/sd_card.h"
#include "../services/sleep_service.h"
#include "../services/storage.h"

namespace {

app::Model model;

// Redraw bookkeeping. needFull is sticky for the frame: any full request wins.
bool     needRender = true;
bool     needFull   = true;
uint32_t recStartMs  = 0;
uint32_t playStartMs = 0;
bool lowBatteryShown = false;
uint32_t lastActivityMs = 0;
bool sleepPending = false;
char currentRecordingDir[20] = {0};  // active recording's directory name, "" when idle

void requestRender(bool full) {
  needRender = true;
  if (full) needFull = true;
}

void go(app::Screen s, bool full) {
  model.screen = s;
  requestRender(full);
}

// ── storage-backed recording list ──────────────────────────────────
void refreshSystemModel() {
  services::battery::Reading b = services::battery::last();
  model.battery = b.valid ? b.percent : -1;
  model.batteryV = b.valid ? b.voltage : 0.0f;
  if (b.low && !lowBatteryShown && model.condition == app::Condition::None) {
    lowBatteryShown = true;
    model.condition = app::Condition::LowBattery;
    requestRender(true);
  }
  if (!b.low) lowBatteryShown = false;
  model.sdPresent = services::sdcard::mounted();
  model.sdFreeGB = services::sdcard::freeGB();
  model.timeSet = services::clock::status().timeSet;
}

void loadRecordingsFromStorage() {
  services::storage::rescan();
  model.recCount = 0;
  const size_t count = services::storage::noteCount();
  for (size_t i = 0; i < count && model.recCount < app::MAX_RECS; ++i) {
    services::storage::NoteSummary n;
    if (!services::storage::noteAt(i, n)) continue;
    app::Rec& r = model.recs[model.recCount++];
    strncpy(r.dir, n.dir, sizeof(r.dir) - 1);
    r.dur = (int)n.durationSec;
    r.bytes = n.byteSize;
    strncpy(r.wavPath, n.wavPath, sizeof(r.wavPath) - 1);
    if (n.hasTimestamp) {
      services::clock::formatLocal(n.createdEpoch, r.date, sizeof(r.date));
    } else {
      strncpy(r.date, "time not set", sizeof(r.date) - 1);
    }
  }
  if (model.listIndex >= model.recCount) model.listIndex = model.recCount > 0 ? model.recCount - 1 : 0;
}

void deleteCurrent() {
  if (model.recCount == 0) return;
  services::storage::deleteNote(model.recs[model.listIndex].dir);
  loadRecordingsFromStorage();
}

// ── button dispatch (typed events from buttons service) ─────────────
void onButtonEvent(buttons::Button button, buttons::Press press) {
  lastActivityMs = millis();
  const bool boot = button == buttons::Button::BOOT;
  const bool pwr = button == buttons::Button::PWR;
  Serial.printf("scribr: button %s %s screen=%d condition=%d\n",
                boot ? "BOOT" : "PWR",
                press == buttons::Press::Short ? "short" : (press == buttons::Press::Long ? "long" : "double"),
                (int)model.screen,
                (int)model.condition);

  if (model.condition != app::Condition::None) {
    if ((model.condition == app::Condition::LowBattery || model.condition == app::Condition::Charging ||
         model.condition == app::Condition::StorageFull || model.condition == app::Condition::TimeNotSet) &&
        pwr && press == buttons::Press::Short) {
      model.condition = app::Condition::None;
      go(model.screen, true);
    } else if (model.condition == app::Condition::NoSd && boot && press == buttons::Press::Long) {
      model.condition = app::Condition::None;
      go(app::Screen::IDLE, true);
    } else if (model.condition == app::Condition::Sleep) {
      model.condition = app::Condition::None;
      sleepPending = false;
      go(app::Screen::IDLE, true);
    }
    return;
  }

  if (boot && press == buttons::Press::Short) {
    switch (model.screen) {
      case app::Screen::REC:
        services::audio::cancelRecording();
        if (currentRecordingDir[0]) services::storage::deleteNote(currentRecordingDir);
        currentRecordingDir[0] = 0;
        go(app::Screen::IDLE, true);
        break;  // cancel: discard
      case app::Screen::LIST:
        if (model.recCount > 0) { model.listIndex = (model.listIndex + 1) % model.recCount; requestRender(true); }
        break;
      case app::Screen::DELETE_CONFIRM: go(app::Screen::LIST, true); break;  // cancel
      default: break;
    }
  } else if (boot && press == buttons::Press::Double) {
    if (model.screen == app::Screen::IDLE) {
      if (!services::storage::available()) { model.condition = app::Condition::NoSd; requestRender(true); }
      else { loadRecordingsFromStorage(); model.listIndex = 0; go(app::Screen::LIST, true); }
    }
  } else if (boot && press == buttons::Press::Long) {
    if (model.screen == app::Screen::LIST) go(app::Screen::IDLE, true);  // exit
  } else if (pwr && press == buttons::Press::Short) {
    switch (model.screen) {
      case app::Screen::REC: {
        uint32_t duration = 0, bytes = 0;
        const bool saved = services::audio::stopRecordingAndSave(duration, bytes);
        const uint32_t wallDuration = max<uint32_t>(1, (millis() - recStartMs + 500) / 1000);
        Serial.printf("scribr: recording stop saved=%d audioDuration=%lu wallDuration=%lu bytes=%lu dir=%s\n",
                      saved ? 1 : 0,
                      (unsigned long)duration,
                      (unsigned long)wallDuration,
                      (unsigned long)bytes,
                      currentRecordingDir);
        if (saved && currentRecordingDir[0]) {
          // Creation time is carried by the directory name; the sidecar only
          // records duration. Use wall-clock duration for metadata/UI: during
          // hardware bring-up byte-derived duration can drift if codec/I2S
          // clocking is not yet final, and the visible REC timer is the
          // acceptance source.
          services::storage::writeMetadata(currentRecordingDir, wallDuration);
        } else if (currentRecordingDir[0]) {
          // Too short / no audio: audio_service removed session.wav, so drop the
          // now-empty directory rather than leaving an orphan on the card.
          services::storage::deleteNote(currentRecordingDir);
        }
        currentRecordingDir[0] = 0;
        loadRecordingsFromStorage();
        go(app::Screen::IDLE, true);
        break;
      }
      case app::Screen::LIST:
        if (model.recCount > 0 && services::audio::startPlayback(model.recs[model.listIndex].wavPath)) {
          model.playElapsed = 0;
          playStartMs = millis();
          go(app::Screen::PLAY, true);
        } else if (model.recCount == 0) {
          go(app::Screen::IDLE, true);
        }
        break;
      case app::Screen::PLAY:
        services::audio::stopPlayback();
        loadRecordingsFromStorage();
        go(app::Screen::LIST, true);
        break;  // stop -> recordings index
      case app::Screen::DELETE_CONFIRM: deleteCurrent(); go(app::Screen::LIST, true); break;  // confirm
      default: break;
    }
  } else if (pwr && press == buttons::Press::Double) {
    if (model.screen == app::Screen::IDLE) {
      if (!services::storage::available()) { model.condition = app::Condition::NoSd; requestRender(true); return; }
      char wav[48], meta[48];
      time_t createdUtc = 0;
      const bool hasTime = services::clock::nowUtc(createdUtc);
      if (!services::storage::newNote(hasTime, createdUtc, currentRecordingDir, sizeof(currentRecordingDir),
                                      wav, sizeof(wav), meta, sizeof(meta))) {
        Serial.println("scribr: could not create recording directory");
        currentRecordingDir[0] = 0;
        model.condition = services::storage::available() ? app::Condition::StorageFull : app::Condition::NoSd;
        requestRender(true);
        return;
      }
      Serial.printf("scribr: recording start dir=%s path=%s\n", currentRecordingDir, wav);
      if (services::audio::startRecording(wav)) {
        model.recElapsed = 0;
        recStartMs = millis();
        go(app::Screen::REC, true);
      } else {
        Serial.println("scribr: recording start failed");
        services::storage::deleteNote(currentRecordingDir);
        currentRecordingDir[0] = 0;
        model.condition = services::storage::available() ? app::Condition::StorageFull : app::Condition::NoSd;
        requestRender(true);
      }
    }
  } else if (pwr && press == buttons::Press::Long) {
    if (model.screen == app::Screen::LIST && model.recCount > 0) go(app::Screen::DELETE_CONFIRM, true);
  }
}

// ── timers for the live screens ─────────────────────────────────────
void serviceTimers() {
  if (model.screen == app::Screen::REC) {
    if (services::audio::state() == services::audio::State::Idle && millis() - recStartMs > 1000) {
      Serial.println("scribr: recording worker stopped unexpectedly; returning to IDLE");
      // Leave the directory as-is: the worker may have committed valid audio, and
      // the next scan ignores any directory without a real session.wav.
      currentRecordingDir[0] = 0;
      go(app::Screen::IDLE, true);
      return;
    }
    int e = (int)((millis() - recStartMs) / 1000);
    if (e != model.recElapsed) { model.recElapsed = e; requestRender(false); }  // partial tick
  } else if (model.screen == app::Screen::PLAY) {
    // Let the audio service be the source of truth for playback completion.
    // Metadata duration can be missing/zero for imported or partially indexed
    // notes; treating e >= dur as EOF immediately bounces PLAYBACK to IDLE.
    int e = (int)services::audio::playbackElapsedSec();
    if (services::audio::state() == services::audio::State::Idle) {
      loadRecordingsFromStorage();
      go(app::Screen::LIST, true);
    } else if (e != model.playElapsed) { model.playElapsed = e; requestRender(false); }
  }
}

}  // namespace

void app::begin() {
  services::sdcard::begin();
  services::config::BootConfig cfg;
  services::config::loadBootConfig(cfg);
  services::clock::begin(cfg);
  services::battery::begin();
  services::storage::begin();
  services::audio::begin();

  refreshSystemModel();
  loadRecordingsFromStorage();
  buttons::begin(onButtonEvent);

  lastActivityMs = millis();
  if (!services::clock::status().timeSet) model.condition = app::Condition::TimeNotSet;
  go(app::Screen::IDLE, true);
}

void app::tick() {
  buttons::tick();
  services::battery::tick();
  // Host can push the real UTC time over USB serial ("TIME <epoch>") at any time.
  // On success, drop the TIME NOT SET screen and redraw with the live clock.
  if (services::clock::pollSerialTimeSet()) {
    lastActivityMs = millis();
    if (model.condition == app::Condition::TimeNotSet) model.condition = app::Condition::None;
    requestRender(true);
  }
  refreshSystemModel();
  serviceTimers();

  if (model.screen == app::Screen::IDLE && model.condition == app::Condition::None &&
      millis() - lastActivityMs >= 120000UL) {
    model.condition = app::Condition::Sleep;
    sleepPending = true;
    requestRender(true);
  }

  // Dispatch a redraw only when the previous refresh has finished (it runs on
  // the other core). Until then buttons keep being serviced above.
  if (needRender && !display::busy()) {
    screens::render(model);
    if (needFull) display::flushFull(); else display::flush();
    needRender = false;
    needFull = false;
  }

  if (sleepPending && model.condition == app::Condition::Sleep && !needRender && !display::busy()) {
    services::sleep::enterLightSleepUntilButton();
    services::sleep::wakeToIdle();
    sleepPending = false;
    model.condition = app::Condition::None;
    lastActivityMs = millis();
    go(app::Screen::IDLE, true);
  }
}
