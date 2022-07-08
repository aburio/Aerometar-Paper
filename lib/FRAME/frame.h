#ifndef _FRAME_H_
#define _FRAME_H_

#include <Adafruit_GFX.h>

typedef enum
{
  WHITE,
  BLACK
} Color;

class Frame : public GFXcanvas1
{
public:
  Frame(uint16_t w, uint16_t h):GFXcanvas1(w, h) {
  }

  void clear() {
    GFXcanvas1::fillScreen(WHITE);
    GFXcanvas1::setCursor(0,0);
  }
};

#endif // _FRAME_H_