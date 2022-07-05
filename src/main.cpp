
#include "bsp.h"
#include "user_wifi.h"

#include "display/display.h"
#include "wifi/wifi.h"

#if !defined(SSID) || !defined(PASSWORD)
#error "Create a user_wifi.h header and add SSID & PASSWORD define to allow wifi connection"
#endif

/* local types & variables */

/* private functions prototypes */

/* public functions */
/**
 * @brief Setup function
 *
 */
void setup()
{
  // logger
  if (CORE_DEBUG_LEVEL == 6)
  {
    Serial.begin(115200);
  }

  // init
  displayInit(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  wifiInit(SSID, PASSWORD);

  // task core 0
  xTaskCreatePinnedToCore(displayTask, "displayTask", 2048, NULL, 1, NULL, 0);

  // task core 1
  xTaskCreatePinnedToCore(wifiTask, "wifiTask", 2048, NULL, 1, NULL, 1);
}

/**
 * @brief Main loop
 *
 */
void loop()
{
}
