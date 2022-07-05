#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "bsp.h"

void displayInit(uint16_t width, uint16_t height);
void displayTask(void *pvParameters);

#endif // _DISPLAY_H_