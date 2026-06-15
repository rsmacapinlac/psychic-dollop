#pragma once
#include "model.h"

// scribr screens. render() composes the current screen into the shared display
// canvas. It does NOT flush — the caller (app) chooses partial vs full refresh.
namespace screens {

void render(const app::Model& m);

}  // namespace screens
