
#include "Display.h"

#include <iostream>
#include <sstream>
#include <string>

// I was tired of carrying those around.
struct Rect {
  int16_t x, y;      
  uint8_t width, height; 
  Rect() = default;
  constexpr Rect(int16_t x, int16_t y, uint8_t width, uint8_t height) 
    : x(x), y(y), width(width), height(height) {}
};


// A simple greedy algorithm to wrap paragraphs.
// * text: some unformatted text to wrap to size
// * max_width: The maximum width available on a line
// * sizing: a function returning the calculated size of a string (depends on the font)
//
// Note: This is far from the Knuth & Plass, this is fairly rustic. Most importantly there is no
// hyphenation, if a word does not fit on one line, it will exceed the alloted size…
//
static std::vector<std::string> wrap(const std::string &text, int16_t max_width, 
                                     std::function<int16_t(const std::string&)> sizing) {  
  std::vector<std::string> lines;
  std::istringstream words(text);
  std::ostringstream next_line;
  std::string word;

  const size_t space_length = sizing(" ");

  if (words >> word) {
    next_line << word;
    size_t space_used = sizing(word);
    
    while (words >> word) {
      size_t word_length = sizing(word);
      if (space_used + space_length + word_length > max_width) {
        lines.push_back(next_line.str());
        next_line.str(""); next_line.clear();
        next_line << word;
        space_used = word_length;
      } else {
        next_line << " " << word;
        space_used += space_length + word_length;
      }
    }
    lines.push_back(next_line.str());
  }
  return lines;
}


Display::Display(GxEPD *e_paper, bool show_battery) : 
     _e_paper(e_paper), _show_battery(show_battery) {
  _u8g2.begin(*_e_paper); // connect u8g2 procedures to Adafruit GFX
  
  if (_show_battery) {
    _top_margin = 30;
    _bottom_margin = 30;
    _text_margin = 3;
  } else {
    _top_margin = 3;
    _bottom_margin = 30;
    _text_margin = 3;
  }
}


void Display::fullRefresh(const std::string &text, int battery_level) {
  _e_paper->fillScreen(GxEPD_WHITE);
  if (_show_battery) {
    _e_paper->drawFastHLine(_text_margin, _top_margin, 
                          _e_paper->width() - _text_margin * 2, GxEPD_BLACK);
    updateBatteryLevel(battery_level, /*erase=*/false);
  }
  _e_paper->drawFastHLine(_text_margin, _e_paper->height() - _bottom_margin,
                        _e_paper->width() - _text_margin * 2, GxEPD_BLACK);
  updateText(text, /*erase=*/false);
  _e_paper->update();
}


void Display::updateBatteryLevel(int battery_level, bool erase) {
  if (!_show_battery) {
    return;
  }

  if (battery_level < 0) {
    battery_level = 0;
  } else if (battery_level > 5) {
    battery_level = 5;
  }
  uint16_t glyph = 0x0030 + battery_level;

  _u8g2.setFont(u8g2_font_battery19_tn);
  _u8g2.setFontDirection(1);              

  int v = (_bottom_margin - _u8g2.getUTF8Width("0")) / 2;
    
  // [TODO] This is incorrect.
  Rect update = Rect(175, v, _u8g2.getUTF8Width("0"), _u8g2.getFontAscent());
  
  if (erase) {
    _e_paper->fillRect(update.x, update.y, update.width, update.height, GxEPD_WHITE);
  }
  
  _u8g2.drawGlyph(175, v, glyph);
  _u8g2.setFontDirection(0);  // left to right (this is default)
  
  if (erase) {
    _e_paper->updateWindow(update.x, update.y, update.width, update.height, false);
  }
}


void Display::updateText(const std::string &text, bool erase) {
  Rect update = Rect(_text_margin,
                     _top_margin + 1,
                     _e_paper->width() - _text_margin * 2, 
                     _e_paper->height() - _top_margin - _bottom_margin - 2);
  
  if (erase) {
    _e_paper->fillRect(update.x, update.y, update.width, update.height, GxEPD_WHITE);
  }
  
  // The fonts to try, from the largest to the smallest. All the sizes will be tried in order until
  // the text fits. 
  const uint8_t *fonts[] = {
    u8g2_font_helvR24_te, u8g2_font_helvR18_te, u8g2_font_helvR14_te,
    u8g2_font_helvR12_te, u8g2_font_helvR10_te, u8g2_font_helvR08_te
  };
  const int fontCount = sizeof(fonts) / sizeof(uint8_t*);

  std::vector<std::string> lines;
  int8_t line_height;
  
  for (int ii = 0; ii < fontCount; ii++) {
    _u8g2.setFont(fonts[ii]);
    line_height = _u8g2.getFontAscent() + (-_u8g2.getFontDescent()) + 7;
    
    auto sizing = [&](const std::string &word) -> int16_t {
        return _u8g2.getUTF8Width(word.c_str());
    };
    lines = wrap(text, update.width, sizing);
    
    // If one of the line is too long, this means a word doesn't fit on one line. In that case, loop
    // and try a smaller font.
    bool too_long = false;
    for (int jj = 0; jj < lines.size(); jj++) {
      if (sizing(lines[jj]) > update.width)
        too_long = true;
    }
    if (too_long)
      continue;  // One word does not fit on one line…

    // Checks if the number of lines fits on the screen. If it does, it's done.
    if (line_height * lines.size() <= update.height)
      break;
  }
  
  // This is not perfect, there are a couple of pathological cases:
  // * If one word is larger than the screen, even with the smallest font, it will be clipped
  // * If the text is too long, the bottom lines will be ignored.
  _u8g2.setForegroundColor(GxEPD_BLACK);  // apply Adafruit GFX color
  _u8g2.setBackgroundColor(GxEPD_WHITE);  // apply Adafruit GFX color
  _u8g2.setFontMode(1);

  for (int ii = 0; ii < lines.size(); ii++) {
    auto vertical_position = _top_margin + _u8g2.getFontAscent() + 5 + line_height * ii;
    _u8g2.drawUTF8(_text_margin, vertical_position, lines[ii].c_str());
    
    if (vertical_position > _e_paper->height() - _bottom_margin)
      break;
  }

  if (erase) {
    _e_paper->updateWindow(update.x, update.y, update.width, update.height, false);
  }
}
