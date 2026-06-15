#pragma once
#include <Adafruit_GFX.h>

// scribr display service.
//
// You draw into a 200x200 1-bit Adafruit GFX canvas (full print()/font API),
// then push it to the panel. The e-paper supports two refresh modes:
//   flush()     — fast PARTIAL refresh, no black flash. Use for small in-place
//                 updates like a ticking timer.
//   flushFull() — FULL refresh (brief black flash) that also clears ghosting
//                 and re-bases partial mode. Use on screen changes.
//
// The canvas and panel framebuffer share an identical layout (200x200,
// MSB-first, 25 bytes/row) but opposite polarity: on the canvas a *set* bit is
// ink; on the panel a set bit is white. The blit handles the inversion.
namespace display {

constexpr int WIDTH  = 200;
constexpr int HEIGHT = 200;

void         begin();      // power on rails, init panel into partial mode
GFXcanvas1&  canvas();     // the 1-bit canvas to draw into (ink = color 1)
void         flush();      // fast partial refresh (async, runs on 2nd core)
void         flushFull();  // full refresh (async): clears ghosting, re-bases partial
bool         busy();       // true while a refresh is still running on the 2nd core

}  // namespace display
