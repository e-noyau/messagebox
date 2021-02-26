#include <string>
#include <iostream>
#include <sstream>
#include <functional>

// For the colors
#include <GxEPD.h>


#include "TextDisplay.h"



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


void TextDisplay::clear() {
  _u8g2.u8g2.gfx->fillRect(_position.x, _position.y,
                           _position.width, _position.height, GxEPD_WHITE);
}


void TextDisplay::update(const std::string &text) {
  // The fonts to try, from the largest to the smallest. All the sizes will be tried in order until
  // the text fits. 
  const uint8_t *fonts[] = {
#if 1
    u8g2_font_helvR24_te, u8g2_font_helvR18_te, u8g2_font_helvR14_te,
    u8g2_font_helvR12_te, u8g2_font_helvR10_te, u8g2_font_helvR08_te
#else
    u8g2_font_helvR14_te, u8g2_font_helvR12_te, u8g2_font_helvR10_te
#endif
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
    lines = wrap(text, _position.width, sizing);
  
    // If one of the line is too long, this means a word doesn't fit on one line. In that case, loop
    // and try a smaller font.
    bool too_long = false;
    for (int jj = 0; jj < lines.size(); jj++) {
      if (sizing(lines[jj]) > _position.width)
        too_long = true;
    }
    if (too_long)
      continue;  // One word does not fit on one line…

    // Checks if the number of lines fits on the screen. If it does, it's done.
    if (line_height * lines.size() <= _position.height)
      break;
  }

  // This is not perfect, there are a couple of pathological cases:
  // * If one word is larger than the screen, even with the smallest font, it will be clipped
  // * If the text is too long, the bottom lines will be ignored.
  //
  _u8g2.setForegroundColor(GxEPD_BLACK);
  //_u8g2.setBackgroundColor(GxEPD_WHITE);  // Not necessary in font mode 1.
  _u8g2.setFontMode(1);

  for (int ii = 0; ii < lines.size(); ii++) {
    auto vertical_position = _position.y + _u8g2.getFontAscent() + 5 + line_height * ii;
    _u8g2.drawUTF8(_position.x, vertical_position, lines[ii].c_str());
  
    if (vertical_position > _position.y + _position.height)
      break;
  }  
}
