#pragma once
#include <stdint.h>

// scribr UI model — the data the screens render. For now this is a UX
// prototype: recordings are fake and timers are simulated (no audio/SD yet).
namespace app {

enum class Screen : uint8_t { IDLE, REC, LIST, PLAY, DELETE_CONFIRM };

struct Rec {
  char date[20];  // "YYYY-MM-DD HH:MM"
  int  dur;       // seconds
};

constexpr int MAX_RECS = 16;

struct Model {
  Screen screen      = Screen::IDLE;
  int    battery     = 84;       // %
  float  sdFreeGB    = 28.6f;
  Rec    recs[MAX_RECS];
  int    recCount    = 0;
  int    listIndex   = 0;        // selected recording in LIST
  int    recElapsed  = 0;        // seconds, while RECording
  int    playElapsed = 0;        // seconds, while PLAYing
};

}  // namespace app
