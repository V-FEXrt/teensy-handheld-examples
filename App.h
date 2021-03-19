#pragma once

#include "System.h"

class App {
public:
  App() = default;

  void init(System& sys);
  void tick(System& sys);

private:
  uint32_t last_draw_ = 0;
  uint16_t brightness_slider_;
  uint16_t last_set_brightness_;
};