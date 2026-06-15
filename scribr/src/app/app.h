#pragma once

// scribr application: the UX state machine + button handling.
// begin() sets up the buttons and paints the first screen; tick() must be
// called every loop() to service buttons, timers, and redraws.
namespace app {

void begin();
void tick();

}  // namespace app
