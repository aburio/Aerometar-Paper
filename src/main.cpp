
#include <Arduino.h>
#include <SPI.h>
#include <EPD.h>
#include <frame.h>

#define DISPLAY_WIDTH 264
#define DISPLAY_HEIGHT 176

static uint8_t state = 0;

EPD einkDisplay(DISPLAY_WIDTH, DISPLAY_HEIGHT, 33, 25, 26, 27, 14, 5, SPI);
Frame displayFrame(DISPLAY_WIDTH, DISPLAY_HEIGHT);

// setup
void setup()
{
  einkDisplay.begin();
  displayFrame.fillScreen(WHITE);
}

// main loop
void loop()
{
  einkDisplay.setFactor();

  switch (state)
  {
  default:
    break;
    
  case 0:
    einkDisplay.clear();
    state = 1;
    break;

  case 1:
    displayFrame.setCursor(100,100);
    displayFrame.setTextColor(1);
    displayFrame.printf("LFLY METAR TAF");
    einkDisplay.update(displayFrame.getBuffer());
    displayFrame.clear();
    state = 2;
    break;

  case 2:
    displayFrame.drawCircle(100,30,20,BLACK);
    einkDisplay.update(displayFrame.getBuffer());
    displayFrame.clear();
    state = 0;
    break;
  }

  delay(10000);
}
