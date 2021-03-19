#include "App.h"

#include <TimeLib.h>

#define TAG_BRIGHTNESS_SLIDER 1

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x200 ? '1' : '0'), \
  (byte & 0x100 ? '1' : '0'), \
  (byte & 0x080 ? '1' : '0'), \
  (byte & 0x040 ? '1' : '0'), \
  (byte & 0x020 ? '1' : '0'), \
  (byte & 0x010 ? '1' : '0'), \
  (byte & 0x008 ? '1' : '0'), \
  (byte & 0x004 ? '1' : '0'), \
  (byte & 0x002 ? '1' : '0'), \
  (byte & 0x001 ? '1' : '0')

char line_buffer[121];

int key_code = -1;

void recieve_key(int key) {
    key_code = key;
}

void App::init(System& sys) {
    brightness_slider_ = 65535;
    sys.set_backlight_brightness(128);

    sys.set_rotation(System::DisplayRotation::LANDSCAPE);

    sys.set_on_key_press(recieve_key);

    last_draw_ = 0;
}

void App::tick(System& sys) {

  // Button service
  if (sys.controller().buttons_changed()) {
    sprintf(line_buffer, "[Buttons] ");
    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_BLUE;
    sys.terminal().append_string(line_buffer);
    sprintf(line_buffer, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(sys.controller().buttons));
    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_CYAN;
    sys.terminal().append_string(line_buffer);
    if ((sys.controller().buttons & 1) == 0) {
      sys.terminal().ring_bell();
    }
  }

  // Check for input key
  if (key_code != -1) {
    sys.terminal().append_string("key: ");
    switch (key_code) {
      case KEYD_UP:        sys.terminal().append_string("UP\n");    break;
      case KEYD_DOWN:      sys.terminal().append_string("DN\n");    break;
      case KEYD_LEFT:      sys.terminal().append_string("LEFT\n");  break;
      case KEYD_RIGHT:     sys.terminal().append_string("RIGHT\n"); break;
      case KEYD_INSERT:    sys.terminal().append_string("Ins\n");   break;
      case KEYD_DELETE:    sys.terminal().append_string("Del\n");   break;
      case KEYD_PAGE_UP:   sys.terminal().append_string("PUP\n");   break;
      case KEYD_PAGE_DOWN: sys.terminal().append_string("PDN\n");   break;
      case KEYD_HOME:      sys.terminal().append_string("HOME\n");  break;
      case KEYD_END:       sys.terminal().append_string("END\n");   break;
      case KEYD_F1:        sys.terminal().append_string("F1\n");    break;
      case KEYD_F2:        sys.terminal().append_string("F2\n");    break;
      case KEYD_F3:        sys.terminal().append_string("F3\n");    break;
      case KEYD_F4:        sys.terminal().append_string("F4\n");    break;
      case KEYD_F5:        sys.terminal().append_string("F5\n");    break;
      case KEYD_F6:        sys.terminal().append_string("F6\n");    break;
      case KEYD_F7:        sys.terminal().append_string("F7\n");    break;
      case KEYD_F8:        sys.terminal().append_string("F8\n");    break;
      case KEYD_F9:        sys.terminal().append_string("F9\n");    break;
      case KEYD_F10:       sys.terminal().append_string("F10\n");   break;
      case KEYD_F11:       sys.terminal().append_string("F11\n");   break;
      case KEYD_F12:       sys.terminal().append_string("F12\n");   break;
      default: {
          sys.terminal().append_character((char)key_code); 
          sys.terminal().append_character('\n'); 
          break;
      }
    }

    key_code = -1;
  }


  // Brightness Sliders
  switch (GD.inputs.track_tag & 0xff) {
  case TAG_BRIGHTNESS_SLIDER: // Check Brightness Slider
    brightness_slider_ = GD.inputs.track_val;
    sys.set_backlight_brightness(floor(((float) brightness_slider_ / 65535.0) * 128));
    break;
  case TAG_SCROLLBAR: // Terminal Scrollbar
    sys.terminal().update_scrollbar_position(GD.inputs.track_val);
    break;
  }

  // Every 5 seconds print the time and battery voltage
  if (millis() > last_draw_ + 5000) {
    // Print time
    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_MAGENTA;
    sys.terminal().background_color = TERMINAL_VGA_BRIGHT_CYAN;
    sprintf(line_buffer, "[Time] ");
    sys.terminal().append_string(line_buffer);

    sys.terminal().foreground_color = TERMINAL_VGA_WHITE;
    sys.terminal().background_color = TERMINAL_VGA_BLACK;
    sprintf(line_buffer, "%02d:%02d:%02d ", hour(), minute(), second());
    sys.terminal().append_string(line_buffer);

    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_WHITE;
    sprintf(line_buffer, "%04d-%02d-%02d\n", year(), month(), day());
    sys.terminal().append_string(line_buffer);

    // Print battery voltage
    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_GREEN;
    sprintf(line_buffer, "[Battery] %1.2f V\n", sys.battery_voltage());
    sys.terminal().append_string(line_buffer);

    last_draw_ = millis();
  } 

  // Check for brightness change
  if (brightness_slider_ != last_set_brightness_) {
    last_set_brightness_ = brightness_slider_;

    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_YELLOW;
    sprintf(line_buffer, "[Brightness] ");
    sys.terminal().append_string(line_buffer);

    sys.terminal().foreground_color = TERMINAL_VGA_BRIGHT_WHITE;
    sprintf(line_buffer, "%3.1f%%\n", sys.screen_brightness());
    sys.terminal().append_string(line_buffer);
  }

  int position_x = 0, position_y = 0;

  // Activates and uses bitmap_handle 14 & 13
  // Be sure to reset the bitmap_handle afterwards
  sys.terminal().draw(position_x, position_y);

  GD.ColorRGB(COLOR_WHITE);

  // Draw Brightness Slider
  GD.Tag(TAG_BRIGHTNESS_SLIDER);
  GD.cmd_slider(8, 2+1*16, GD.w - 2*16, 8, OPT_FLAT,
                brightness_slider_, 65535);
  GD.cmd_track(8, 2+1*16, GD.w - 2*16, 8, TAG_BRIGHTNESS_SLIDER);

} 
