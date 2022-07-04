#ifndef FRAME_H_
#define FRAME_H_

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
  }
};

#endif // FRAME_H_