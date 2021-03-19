#include "System.h"

// Standard GD3 Chip Select Pins
#define GD3_CS 8
#define GD3_SDCS 9

// USB Host
USBHost myusb;
USBHub hub1= USBHub(myusb);
USBHub hub2= USBHub(myusb);
KeyboardController keyboard1= KeyboardController(myusb);
KeyboardController keyboard2= KeyboardController(myusb);
USBHIDParser hid1 = USBHIDParser(myusb);
USBHIDParser hid2 = USBHIDParser(myusb);
USBHIDParser hid3 = USBHIDParser(myusb);
USBHIDParser hid4 = USBHIDParser(myusb);
USBHIDParser hid5 = USBHIDParser(myusb);
MouseController mouse = MouseController(myusb);
DigitizerController tablet = DigitizerController(myusb);
JoystickController joystick = JoystickController(myusb);
BluetoothController bluet = BluetoothController(myusb, true, "0000");   // Version does pairing to device
RawHIDController rawhid2 = RawHIDController(myusb);

// USB Support

// Lets only include in the lists The most top level type devices we wish to show information for.
USBDriver *drivers[6] = {&keyboard1, &keyboard2, &joystick, &bluet, &hid1, &hid2};

#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"KB1", "KB2", "Joystick(device)", "Bluet", "HID1" , "HID2"};
bool driver_active[CNT_DEVICES] = {false, false, false, false, false};

// Lets also look at HID Input devices
USBHIDInput *hiddrivers[4] = {&tablet, &joystick, &mouse, &rawhid2};
#define CNT_HIDDEVICES (sizeof(hiddrivers)/sizeof(hiddrivers[0]))
const char * hid_driver_names[CNT_HIDDEVICES] = {"tablet", "joystick", "mouse", "RawHid2"};

bool hid_driver_active[CNT_HIDDEVICES] = {false, false, false, false};

BTHIDInput *bthiddrivers[2] = {&joystick, &mouse};
#define CNT_BTHIDDEVICES (sizeof(bthiddrivers)/sizeof(bthiddrivers[0]))
const char * bthid_driver_names[CNT_HIDDEVICES] = {"joystick", "mouse"};
bool bthid_driver_active[CNT_HIDDEVICES] = {false, false};

// Save away values for buttons, x, y, wheel, wheelh
int buttons_cur = 0;
int x_cur = 0,
    y_cur = 0,
    z_cur = 0;
int x2_cur = 0,
    y2_cur = 0,
    z2_cur = 0,
    L1_cur = 0,
    R1_cur = 0;
int wheel_cur = 0;
int wheelH_cur = 0;
int axis_cur[10];

String p = "KB1";
bool BT = 0;

int user_axis[64];
uint32_t buttons_prev = 0;
uint32_t buttons;

bool show_changed_only = false;
bool new_device_detected = false;
int16_t y_position_after_device_info = 0;

uint8_t joystick_left_trigger_value = 0;
uint8_t joystick_right_trigger_value = 0;
uint64_t joystick_full_notify_mask = (uint64_t) - 1;

bool OnReceiveHidData(uint32_t usage, const uint8_t *data, uint32_t len);
void OnPress(int key);
void OnHIDExtrasRelease(uint32_t top, uint16_t key);
void OnHIDExtrasPress(uint32_t top, uint16_t key);

void ProcessJoystickData();
void ProcessMouseData();
void ProcessTabletData();
void UpdateActiveDeviceInfo();

// external callback functions
void (*on_key_press_callback)(int unicode) = NULL;

void System::set_on_key_press(void (*f)(int unicode)) {
  on_key_press_callback = f;
}

void System::init(void) {

    pinMode(BATTERY_VOLTAGE_DIVIDER_PIN, INPUT);

    SPI.begin();

    // Gameduino3 Setup
    GD.begin(~GD_STORAGE, GD3_CS, GD3_SDCS);

    controller_.begin();

    // terminal configuration
    terminal_.begin(TEXTVGA);
    terminal_.set_window_bg_color(0x000000);  // Background window color
    terminal_.set_window_opacity(128);  // 0-255
    terminal_.set_font_vga();
    terminal_.disable_vga_background_colors();
    terminal_.set_size_fullscreen();

    GD.ClearColorRGB(0x000000);

    myusb.begin();
    rawhid2.attachReceive(OnReceiveHidData);
    keyboard1.attachPress(OnPress);
    keyboard2.attachPress(OnPress);
    keyboard1.attachExtrasPress(OnHIDExtrasPress);
    keyboard1.attachExtrasRelease(OnHIDExtrasRelease);
    keyboard2.attachExtrasPress(OnHIDExtrasPress);
    keyboard2.attachExtrasRelease(OnHIDExtrasRelease);
}

void System::tick(void) {

    // USB Host
    myusb.Task();
    UpdateActiveDeviceInfo(); // Update the display with
    ProcessTabletData(); // Now lets try displaying Tablet data
    ProcessJoystickData(); // And joystick data
    ProcessMouseData(); // Process Mouse Data

    controller_.update_buttons();
}



//=============================================================================
// ProcessMouseData
//=============================================================================
bool OnReceiveHidData(uint32_t usage, const uint8_t *data, uint32_t len) {
  // Called for maybe both HIDS for rawhid basic test.  One is for the Teensy
  // to output to Serial. while still having Raw Hid...
  if (usage == 0xFF000080
     ) {
    //Serial.print("RawHIDx data: ");
    //Serial.println(usage, HEX);
    for (uint32_t j = 0; j < len; j++) {
      user_axis[j] = data[j];
    }

    bool something_changed = false;
    if (user_axis[3] != buttons_cur) {
      buttons_cur = user_axis[3];
      something_changed = true;
    }
    if (user_axis[4] != x_cur) {
      x_cur = user_axis[4];
      something_changed = true;
    }
    if (user_axis[5] != y_cur) {
      y_cur = user_axis[5];
      something_changed = true;
    }
    if (tablet.getWheel() != wheel_cur) {
      wheel_cur = 0;
      something_changed = true;
    }
    if (tablet.getWheelH() != wheelH_cur) {
      wheelH_cur = 0;
      something_changed = true;
    }
    if (something_changed) {
    // TODO:
    //   sprintf(l_buffer,
    //           "%d %d %d %d %d\n", buttons_cur, x_cur, y_cur, wheel_cur, wheelH_cur);
    //   sys.terminal().append_string(l_buffer);
    }
  } else {
    // Lets trim off trailing null characters.
    while ((len > 0) && (data[len - 1] == 0)) {
      len--;
    }
    if (len) {
      //Serial.print("RawHid Serial: ");
      //Serial.write(data, len);
    }
  }
  return true;
}

void OnPress(int key)
{
    if (on_key_press_callback != NULL) {
        (*on_key_press_callback)(key);
    }
}

void OnHIDExtrasPress(uint32_t top, uint16_t key)
{
}

void OnHIDExtrasRelease(uint32_t top, uint16_t key)
{
}

//=============================================================================
// UpdateActiveDeviceInfo
//=============================================================================
void UpdateActiveDeviceInfo() {
  // First see if any high level devices
  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        new_device_detected = true;
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;
        // TODO:
        // sprintf(l_buffer,
        //         "Device %s %x:%x\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        // sys.terminal().append_string(l_buffer);

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  manufacturer: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = drivers[i]->product();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  product: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = drivers[i]->serialNumber();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  Serial: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
      }
    }
  }
  // Then Hid Devices
  for (uint8_t i = 0; i < CNT_HIDDEVICES; i++) {
    if (*hiddrivers[i] != hid_driver_active[i]) {
      if (hid_driver_active[i]) {
        Serial.printf("*** HID Device %s - disconnected ***\n", hid_driver_names[i]);
        hid_driver_active[i] = false;
      } else {
        new_device_detected = true;
        Serial.printf("*** HID Device %s %x:%x - connected ***\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        hid_driver_active[i] = true;
        // TODO:
        // sprintf(l_buffer, "HID Device %s %x:%x\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        // sys.terminal().append_string(l_buffer);

        const uint8_t *psz = hiddrivers[i]->manufacturer();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  manufacturer: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = hiddrivers[i]->product();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  product: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = hiddrivers[i]->serialNumber();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  Serial: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
      }
    }
  }

  // Then Bluetooth devices
  for (uint8_t i = 0; i < CNT_BTHIDDEVICES; i++) {
    if (*bthiddrivers[i] != bthid_driver_active[i]) {
      if (bthid_driver_active[i]) {
        Serial.printf("*** BTHID Device %s - disconnected ***\n", hid_driver_names[i]);
        bthid_driver_active[i] = false;
      } else {
        new_device_detected = true;
        Serial.printf("*** BTHID Device %s %x:%x - connected ***\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        bthid_driver_active[i] = true;
        // TODO:
        // sprintf(l_buffer, "Bluetooth Device %s %x:%x\n", bthid_driver_names[i], bthiddrivers[i]->idVendor(), bthiddrivers[i]->idProduct());
        // sys.terminal().append_string(l_buffer);

        const uint8_t *psz = bthiddrivers[i]->manufacturer();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  manufacturer: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = bthiddrivers[i]->product();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  product: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
        psz = bthiddrivers[i]->serialNumber();
        if (psz && *psz) {
        // TODO:
        //   sprintf(l_buffer, "  Serial: %s\n", psz);
        //   sys.terminal().append_string(l_buffer);
        }
      }
    }
  }
}

//=============================================================================
// ProcessTabletData
//=============================================================================
void ProcessTabletData() {
  if (tablet.available()) {
    if (new_device_detected) {
      // Lets display the titles.
      // int16_t x;
      // tft.getCursor(&x, &y_position_after_device_info);
      // tft.setTextColor(ILI9341_YELLOW);
      // tft.printf("Buttons:\nX:\nY:\nWheel:\nWheel H:\nAxis:");
      new_device_detected = false;
    }
    bool something_changed = false;
    if (tablet.getButtons() != buttons_cur) {
      buttons_cur = tablet.getButtons();
      something_changed = true;
    }
    if (tablet.getMouseX() != x_cur) {
      x_cur = tablet.getMouseX();
      something_changed = true;
    }
    if (tablet.getMouseY() != y_cur) {
      y_cur = tablet.getMouseY();
      something_changed = true;
    }
    if (tablet.getWheel() != wheel_cur) {
      wheel_cur = tablet.getWheel();
      something_changed = true;
    }
    if (tablet.getWheelH() != wheelH_cur) {
      wheelH_cur = tablet.getWheelH();
      something_changed = true;
    }
    // BUGBUG:: play with some Axis...
    for (uint8_t i = 0; i < 10; i++) {
      int axis = tablet.getAxis(i);
      if (axis != axis_cur[i]) {
        axis_cur[i] = axis;
        something_changed = true;
      }
    }

    if (something_changed) {
#define TABLET_DATA_X 100
      // int16_t x, y2;
      // tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      //tft.setTextDatum(BR_DATUM);
      // int16_t y = y_position_after_device_info;
      // tft.setCursor(TABLET_DATA_X, y);
      // tft.printf("%d(%x)", buttons_cur, buttons_cur);
      // tft.getCursor(&x, &y2);
      // tft.fillRect(x, y, 320, line_space, ILI9341_BLACK);

      // y += line_space; OutputNumberField(TABLET_DATA_X, y, x_cur, 320);
      // y += line_space; OutputNumberField(TABLET_DATA_X, y, y_cur, 320);
      // y += line_space; OutputNumberField(TABLET_DATA_X, y, wheel_cur, 320);
      // y += line_space; OutputNumberField(TABLET_DATA_X, y, wheelH_cur, 320);

      // Output other Axis data
      for (uint8_t i = 0; i < 9; i += 3) {
        // y += line_space;
        // OutputNumberField(TABLET_DATA_X, y, axis_cur[i], 75);
        // OutputNumberField(TABLET_DATA_X + 75, y, axis_cur[i + 1], 75);
        // OutputNumberField(TABLET_DATA_X + 150, y, axis_cur[i + 2], 75);
      }
      // tft.updateScreen(); // update the screen now

    }
    tablet.digitizerDataClear();
  }
}

//=============================================================================
// OutputNumberField
//=============================================================================
void OutputNumberField(int16_t x, int16_t y, int val, int16_t field_width) {
  // int16_t x2, y2;
  // tft.setCursor(x, y);
  // tft.print(val, DEC);
  // tft.getCursor(&x2, &y2);
  // tft.fillRect(x2, y, field_width - (x2 - x), Arial_12.line_space, ILI9341_BLACK);
}

//=============================================================================
// ProcessMouseData
//=============================================================================
void ProcessMouseData() {
  if (mouse.available()) {
    if (new_device_detected) {
      // Lets display the titles.
      // int16_t x;
      // tft.getCursor(&x, &y_position_after_device_info);
      // tft.setTextColor(ILI9341_YELLOW);
      // tft.printf("Buttons:\nX:\nY:\nWheel:\nWheel H:");
      new_device_detected = false;
    }

    bool something_changed = false;
    if (mouse.getButtons() != buttons_cur) {
      buttons_cur = mouse.getButtons();
      something_changed = true;
    }
    if (mouse.getMouseX() != x_cur) {
      x_cur = mouse.getMouseX();
      something_changed = true;
    }
    if (mouse.getMouseY() != y_cur) {
      y_cur = mouse.getMouseY();
      something_changed = true;
    }
    if (mouse.getWheel() != wheel_cur) {
      wheel_cur = mouse.getWheel();
      something_changed = true;
    }
    if (mouse.getWheelH() != wheelH_cur) {
      wheelH_cur = mouse.getWheelH();
      something_changed = true;
    }
    if (something_changed) {
#define MOUSE_DATA_X 100
      // int16_t x, y2;
      // unsigned char line_space = Arial_12.line_space;
      // tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      //tft.setTextDatum(BR_DATUM);
      // int16_t y = y_position_after_device_info;
      // tft.setCursor(TABLET_DATA_X, y);
      // tft.printf("%d(%x)", buttons_cur, buttons_cur);
      // tft.getCursor(&x, &y2);
      // tft.fillRect(x, y, 320, line_space, ILI9341_BLACK);

      // y += line_space; OutputNumberField(MOUSE_DATA_X, y, x_cur, 320);
      // y += line_space; OutputNumberField(MOUSE_DATA_X, y, y_cur, 320);
      // y += line_space; OutputNumberField(MOUSE_DATA_X, y, wheel_cur, 320);
      // y += line_space; OutputNumberField(MOUSE_DATA_X, y, wheelH_cur, 320);
      // tft.updateScreen(); // update the screen now
    }

    mouse.mouseDataClear();
  }
}

//=============================================================================
// ProcessJoystickData
//=============================================================================
void ProcessJoystickData() {
  if (joystick.available()) {
    uint64_t axis_mask = joystick.axisMask();
    uint64_t axis_changed_mask = joystick.axisChangedMask();
    Serial.print("Joystick: buttons = ");
    buttons = joystick.getButtons();
    Serial.print(buttons, HEX);
    //Serial.printf(" AMasks: %x %x:%x", axis_mask, (uint32_t)(user_axis_mask >> 32), (uint32_t)(user_axis_mask & 0xffffffff));
    //Serial.printf(" M: %lx %lx", axis_mask, joystick.axisChangedMask());
    if (show_changed_only) {
      for (uint8_t i = 0; axis_changed_mask != 0; i++, axis_changed_mask >>= 1) {
        if (axis_changed_mask & 1) {
          Serial.printf(" %d:%d", i, joystick.getAxis(i));
        }
      }
    } else {
      for (uint8_t i = 0; axis_mask != 0; i++, axis_mask >>= 1) {
        if (axis_mask & 1) {
          Serial.printf(" %d:%d", i, joystick.getAxis(i));
        }
      }
    }
    for (uint8_t i = 0; i < 64; i++) {
      user_axis[i] = joystick.getAxis(i);
    }
    uint8_t ltv;
    uint8_t rtv;
    switch (joystick.joystickType()) {
      default:
        break;
      case JoystickController::PS4:
        ltv = joystick.getAxis(3);
        rtv = joystick.getAxis(4);
        if ((ltv != joystick_left_trigger_value) || (rtv != joystick_right_trigger_value)) {
          joystick_left_trigger_value = ltv;
          joystick_right_trigger_value = rtv;
          joystick.setRumble(ltv, rtv);
        }
        break;

      case JoystickController::PS3:
        ltv = joystick.getAxis(18);
        rtv = joystick.getAxis(19);
        if ((ltv != joystick_left_trigger_value) || (rtv != joystick_right_trigger_value)) {
          joystick_left_trigger_value = ltv;
          joystick_right_trigger_value = rtv;
          joystick.setRumble(ltv, rtv, 50);
        }
        break;

      case JoystickController::XBOXONE:
      case JoystickController::XBOX360:
        ltv = joystick.getAxis(4);
        rtv = joystick.getAxis(5);
        if ((ltv != joystick_left_trigger_value) || (rtv != joystick_right_trigger_value)) {
          joystick_left_trigger_value = ltv;
          joystick_right_trigger_value = rtv;
          joystick.setRumble(ltv, rtv);
          Serial.printf(" Set Rumble %d %d", ltv, rtv);
        }
        break;
    }
    if (buttons != buttons_cur) {
      if (joystick.joystickType() == JoystickController::PS3) {
        joystick.setLEDs((buttons >> 12) & 0xf); //  try to get to TRI/CIR/X/SQuare
      } else {
        uint8_t lr = (buttons & 1) ? 0xff : 0;
        uint8_t lg = (buttons & 2) ? 0xff : 0;
        uint8_t lb = (buttons & 4) ? 0xff : 0;
        joystick.setLEDs(lr, lg, lb);
      }
      buttons_cur = buttons;
    }
    Serial.println();
    // tft_JoystickData();
    joystick.joystickDataClear();
  }
}