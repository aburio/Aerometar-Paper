#include "display.h"

#include <SPI.h>
#include <EPD.h>

/* local types & variables */
QueueHandle_t display_refresh_queue;
Frame *display_frame;
EPD *display_eink;


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
    // init variables
    display_refresh_queue = xQueueCreate(1, sizeof(bool));
    display_eink = new EPD(width, height, 33, 25, 26, 27, 14, 5, SPI);
    display_frame = new Frame(width, height);
    
    // setup display
    display_eink->begin();
    display_frame->clear();
    display_frame->setFont(&Ubuntu_Bold7pt7b);
    display_eink->setFactor();
    display_eink->clear();
    log_v("display_init : init done");
}

/**
 * @brief Task to control the display interface
 *
 * @param pvParameters
 */
void displayTask(void *pvParameters)
{
    BaseType_t status;
    bool display_refresh = false;

    log_v("display_task : start task");
    
    for(;;)
    {
        status = xQueueReceive(display_refresh_queue, &display_refresh, 0);

        if (status == pdPASS)
        {
            display_eink->setFactor();
            display_eink->update(display_frame->getBuffer());
        }

        vTaskDelay(200/portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/* private functions */