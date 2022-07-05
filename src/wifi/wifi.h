#ifndef _WIFI_H_
#define _WIFI_H_

#include "bsp.h"

void wifiInit(const String ssid, const String password);
void wifiTask(void *pvParameters);

#endif // _WIFI_H_