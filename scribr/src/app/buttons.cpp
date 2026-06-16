#include "buttons.h"

#include <OneButton.h>

#include "../hal/pins.h"

namespace {
constexpr int DOUBLE_CLICK_MS = 200;
constexpr int LONG_PRESS_MS = 600;
constexpr int DEBOUNCE_MS = 50;

OneButton btnBoot(BTN_BOOT_PIN, true, true);
OneButton btnPwr(BTN_PWR_PIN, true, true);
buttons::Handler eventHandler = nullptr;

void emit(buttons::Button button, buttons::Press press) {
  if (eventHandler) eventHandler(button, press);
}

void bootClick() { emit(buttons::Button::BOOT, buttons::Press::Short); }
void bootDouble() { emit(buttons::Button::BOOT, buttons::Press::Double); }
void bootLong() { emit(buttons::Button::BOOT, buttons::Press::Long); }

void pwrClick() { emit(buttons::Button::PWR, buttons::Press::Short); }
void pwrDouble() { emit(buttons::Button::PWR, buttons::Press::Double); }
void pwrLong() { emit(buttons::Button::PWR, buttons::Press::Long); }
}  // namespace

namespace buttons {

void begin(Handler handler) {
  eventHandler = handler;

  btnBoot.setClickMs(DOUBLE_CLICK_MS);
  btnBoot.setPressMs(LONG_PRESS_MS);
  btnBoot.setDebounceMs(DEBOUNCE_MS);
  btnBoot.attachClick(bootClick);
  btnBoot.attachDoubleClick(bootDouble);
  btnBoot.attachLongPressStart(bootLong);

  btnPwr.setClickMs(DOUBLE_CLICK_MS);
  btnPwr.setPressMs(LONG_PRESS_MS);
  btnPwr.setDebounceMs(DEBOUNCE_MS);
  btnPwr.attachClick(pwrClick);
  btnPwr.attachDoubleClick(pwrDouble);
  btnPwr.attachLongPressStart(pwrLong);
}

void tick() {
  btnBoot.tick();
  btnPwr.tick();
}

}  // namespace buttons
