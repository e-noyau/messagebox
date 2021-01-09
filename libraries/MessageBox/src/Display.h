#pragma once

#include "../../TTGO_TWatch_Library/src/libraries/GxEPD/src/GxEPD.h"
#include "../../TTGO_TWatch_Library/src/libraries/U8g2_for_Adafruit_GFX/src/U8g2_for_Adafruit_GFX.h"

// This class controls the whole display of the message box. It shows indicators in the corners
// and make sure the text is written and erased as needed.
class Display {
public:
  Display(GxEPD *e_paper, bool show_battery = true);
  ~Display() {};
  
  // Reinitialize the whole screen.
  void fullRefresh(const std::string &text, int battery_level);

  // Update just the area with the text
  void updateText(const std::string &text, bool erase = true);

  // Update just the area with the battery level
  void updateBatteryLevel(int battery_level, bool erase = true);  // 0 to 5

private:
  GxEPD *_e_paper;
  U8G2_FOR_ADAFRUIT_GFX _u8g2;
  
  bool _show_battery;
  
  int _top_margin;
  int _bottom_margin;
  int _line_margin;
  int _text_margin;
};
