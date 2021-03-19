#pragma once

#include <Arduino.h>
#include <USBHost_t36.h>

#include "SPI.h"
#include "GD2.h"
#include "GD2Terminal.h"
#include "Input.h"

#define BATTERY_VOLTAGE_DIVIDER_PIN A8

class System {
public:
  System() = default;

  enum DisplayRotation {
    LANDSCAPE,
    LANDSCAPE_REVERSED,
    PORTRAIT,
    PORTRAIT_REVERSED,
  };

  void init();
  void tick();

  GDClass& gd() {
      return GD;
  }

  GD2Terminal& terminal() {
      return terminal_;
  }

  Input& controller() {
      return controller_;
  }

  float battery_voltage() {
      return ((float) analogRead(BATTERY_VOLTAGE_DIVIDER_PIN)/1024)*3.3*2;
  }

  float screen_brightness() {
    return screen_brightness_ * 0.78125;
  }

  // Backlight PWM: 0-128
  void set_backlight_brightness(uint8_t pwm) {
    uint8_t new_screen_brightness;
    if (pwm > 128) {
      new_screen_brightness = 128;
    } else if (pwm < 1) {
      new_screen_brightness = 1;
    } else {
      new_screen_brightness = pwm;
    }

    if (new_screen_brightness != screen_brightness_) {
      screen_brightness_ = new_screen_brightness;
      GD.cmd_regwrite(REG_PWM_DUTY, screen_brightness_);
    }
  }

  void set_rotation(DisplayRotation r) {
    rotation_ = r;
    GD.cmd_setrotate(r);
  }

  DisplayRotation rotation() {
    return rotation_;
  }

  void set_on_key_press(void (*f)(int unicode));

private:

  GDClass GD_;
  GD2Terminal terminal_;
  Input controller_;

  uint8_t screen_brightness_;
  DisplayRotation rotation_;

};