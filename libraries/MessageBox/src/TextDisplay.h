#include <GxEPD.h>
#include <U8g2_for_Adafruit_GFX.h>


struct Rect {
  int16_t x, y;      
  uint8_t width, height; 
  Rect() = default;
  constexpr Rect(int16_t x, int16_t y, uint8_t width, uint8_t height) 
    : x(x), y(y), width(width), height(height) {}
};


// This class controls the whole display of the message box. It shows indicators in the corners
// and make sure the text is written and erased as needed.
class TextDisplay {
public:
  TextDisplay(GxEPD &e_paper) {
    _u8g2.begin(e_paper);
  };
  ~TextDisplay() {};
  
  void setDisplayPosition(const Rect &position) {
    _position = position;
  };
  
  // Update just the area with the text. To replace existing text, use clear first.
  void update(const std::string &text);
  
  // clear the text area
  void clear();

  const Rect& position() {
    return _position;
  }
private:
  U8G2_FOR_ADAFRUIT_GFX _u8g2;
  Rect _position;
};
