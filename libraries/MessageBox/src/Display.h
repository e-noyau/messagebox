#pragma once

#include "../../TTGO_TWatch_Library/src/libraries/GxEPD/src/GxEPD.h"
#include "../../TTGO_TWatch_Library/src/libraries/U8g2_for_Adafruit_GFX/src/U8g2_for_Adafruit_GFX.h"

// This class controls the whole display of the message box. It shows indicators in the corners
// and make sure the text is written and erased as needed.
class Display {
public:
  Display(GxEPD *ePaper);
  ~Display() {};
  
  void fullRefresh(const std::string &text, int battery_level);
  void updateText(const std::string &text, bool erase = true);
  void updateBatteryLevel(int battery_level, bool erase = true);  // 0 to 5
private:
  GxEPD *ePaper;
  U8G2_FOR_ADAFRUIT_GFX u8g2; 
};
