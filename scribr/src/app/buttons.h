#pragma once

// Runtime button input service. Owns OneButton configuration and emits typed
// events to the app state machine.
namespace buttons {

enum class Button { BOOT, PWR };
enum class Press { Short, Long, Double };

using Handler = void (*)(Button button, Press press);

void begin(Handler handler);
void tick();

}  // namespace buttons
