#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "bsp.h"
#include <frame.h>
#include <Fonts/UbuntuRegular6pt7b.h>
#include <Fonts/UbuntuBold7pt7b.h>

/* public types & variables */
extern QueueHandle_t display_refresh_queue;
extern Frame *display_frame;

/* public functions prototypes */
void displayInit(uint16_t width, uint16_t height);
void displayTask(void *pvParameters);

#endif // _DISPLAY_H_