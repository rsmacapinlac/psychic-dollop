#pragma once
#include <stdint.h>

// scribr UI model — the data the screens render. For now this is a UX
// prototype: recordings are fake and timers are simulated (no audio/SD yet).
namespace app {

enum class Screen : uint8_t { IDLE, REC, LIST, PLAY, DELETE_CONFIRM };
enum class Condition : uint8_t { None, LowBattery, Charging, NoSd, StorageFull, TimeNotSet, Sleep };

struct Rec {
  char dir[20] = {0};  // recording directory name, e.g. "20260618T091500"
  char date[20];      // "YYYY-MM-DD HH:MM" or "time not set"
  char wavPath[48];   // "/recordings/<dir>/session.wav"
  int  dur = 0;       // seconds
  uint32_t bytes = 0;
};

constexpr int MAX_RECS = 16;

struct Model {
  Screen screen      = Screen::IDLE;
  Condition condition = Condition::None;
  int    battery     = -1;       // %, -1 when unknown
  float  batteryV    = 0.0f;     // measured pack voltage for gauge calibration
  float  sdFreeGB    = 0.0f;
  bool   sdPresent   = false;
  bool   timeSet     = false;
  Rec    recs[MAX_RECS];
  int    recCount    = 0;
  int    listIndex   = 0;        // selected recording in LIST
  int    recElapsed  = 0;        // seconds, while RECording
  int    playElapsed = 0;        // seconds, while PLAYing
};

}  // namespace app
