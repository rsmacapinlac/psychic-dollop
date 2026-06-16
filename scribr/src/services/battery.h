#pragma once

namespace services::battery {

struct Reading {
  float voltage = -1.0f;
  int percent = -1;
  bool valid = false;
  bool low = false;
};

void begin();
Reading readNow();
Reading last();
void tick();

}  // namespace services::battery
