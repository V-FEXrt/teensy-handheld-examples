#include <Arduino.h>

#include <USBHost_t36.h>

#include <stdlib.h>
#include <stdio.h>

#include "Input.h"

#include "App.h"
#include "System.h"

System sys;
App app;

char l_buffer[121];

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  sys.init();
  app.init(sys);
}

// Display List size
uint16_t display_list_offset = 0;

// FPS and time per game loop
uint32_t delta_time_start = 0;
uint32_t delta_time_micros = 30000; // initial guess
uint32_t frame_time = 0;
int frame_count = 0;
int last_frame_count = 0;

// Loop Time
// 60 fps = 16667 micros
// 30 fps = 33334 micros
uint32_t desired_frame_time = 16667;

void loop() {
  // Frame time start
  delta_time_start = micros();

  // Get touchscreen inputs
  GD.get_inputs();

  // Update Controller
  sys.tick();

  // Draw Screen
  GD.Clear();

  // Update the app
  app.tick(sys);

  // Draw FPS and Display List Usage for the previous frame.
  float delta_time = (float)delta_time_micros * 1e-6;

  if (millis() > frame_time + 1000) {
    last_frame_count = frame_count;
    frame_count = 0;
    frame_time = millis();
  }

  sprintf(l_buffer,
          "Fex: Loop: %lu -- FPS: %.2f (%d) -- DisplayList: %3.2f%% (%u / 8192, %d instructions) ",
          delta_time_micros,
          1.0 / delta_time,
          (int)last_frame_count,
          100.0 * ((float)display_list_offset/8192),
          display_list_offset,
          (int)floor(display_list_offset/4));
  GD.ColorRGB(COLOR_LIGHT_STEEL_BLUE);
  GD.cmd_text(1, GD.h-16,
              20, 0,
              l_buffer);

  // Get the size (current position) of the display list.
  GD.finish();
  display_list_offset = GD.rd16(REG_CMD_DL);

  // Frame time end.
  delta_time_micros = micros() - delta_time_start;

  if (delta_time_micros < desired_frame_time) {
    delay(floor((desired_frame_time - delta_time_micros) * 0.001));
    delta_time_micros = micros() - delta_time_start;
  }

  GD.swap();
  frame_count++;
}