#include "display.h"

#include <SPI.h>
#include <EPD.h>
#include <frame.h>

/* local types & variables */
Frame *displayFrame;
EPD *einkDisplay;
uint8_t state;

/* private functions prototypes */

/* public functions */
/**
 * @brief Initialize the display interface
 *
 * @param width display width in pixels
 * @param height display height in pixels
 */
void displayInit(uint16_t width, uint16_t height)
{
    displayFrame = new Frame(width, height);
    einkDisplay = new EPD(width, height, 33, 25, 26, 27, 14, 5, SPI);
    
    // init
    einkDisplay->begin();
    displayFrame->fillScreen(WHITE);
    state=0;
    log_v("display_init : init done");
}

/**
 * @brief Task to control the display interface
 *
 * @param pvParameters
 */
void displayTask(void *pvParameters)
{
    
    for(;;)
    {
        einkDisplay->setFactor();

        switch (state)
        {
        default:
            vTaskDelay(100/portTICK_PERIOD_MS);
            break;

        case 0:
            log_v("display_task : clear screen");
            einkDisplay->clear();
            state = 1;
            break;

        case 1:
            log_v("display_task : draw text");
            displayFrame->setCursor(100, 100);
            displayFrame->setTextColor(1);
            displayFrame->printf("LFLY METAR TAF");
            einkDisplay->update(displayFrame->getBuffer());
            displayFrame->clear();
            state = 2;
            break;

        case 2:
            log_v("display_task : draw circle");
            displayFrame->drawCircle(100, 30, 20, BLACK);
            einkDisplay->update(displayFrame->getBuffer());
            displayFrame->clear();
            state = 0;
            break;
        }

        vTaskDelay(3000/portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/* private functions */