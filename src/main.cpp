#include "bsp.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "display/display.h"
#include "wifi/wifi.h"
#include "user_wifi.h"

#if !defined(SSID) || !defined(PASSWORD)
#error "Create a user_wifi.h header and add SSID & PASSWORD define to allow wifi connection"
#endif

/* local types & variables */

/* private functions prototypes */
String getDate()
{
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char date[11];

  strftime(&date[0],11,"%F",&timeinfo);

  return String(date);
}

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

  // task core 1
  xTaskCreatePinnedToCore(displayTask, "displayTask", 2048, NULL, 1, NULL, 1);

  // task core 0
  xTaskCreatePinnedToCore(wifiTask, "wifiTask", 2048, NULL, 1, NULL, 0);
}

/**
 * @brief Main loop
 *
 */
void loop()
{
  BaseType_t queue_status;
  int8_t wifi_status;
  bool display_refresh = true;
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();

  if (wifi_event_queue != NULL && display_refresh_queue != NULL)
  {
    queue_status = xQueueReceive(wifi_event_queue, &wifi_status, 0);

    if (queue_status == pdPASS)
    {
      if (wifi_status == 3)
      {
        configTime(0, 0, "pool.ntp.org");
      }
      log_v("loop : wifi state %d", wifi_status);
      display_frame->clear();
      display_frame->setCursor(0,10);
      display_frame->printf("Wifi state %d\r\n", wifi_status);
      xQueueSend(display_refresh_queue, &display_refresh, 0);
    }

    if (wifi_status == 3)
    {
      int16_t code = 0;
      String date = getDate();

      https.begin(client, "https://api.met.no/weatherapi/tafmetar/1.0/metar?&date=" + date + "&icao=LFLY");

      code = https.GET();
      if (code == HTTP_CODE_OK)
      {
        String payload = https.getString();
        log_v("loop : %s", payload.substring(payload.lastIndexOf("LFLY")).c_str());
        display_frame->clear();
        display_frame->setCursor(0,10);
        display_frame->setFont(&Ubuntu_Bold7pt7b);
        display_frame->println("METAR");
        display_frame->setFont(&Ubuntu_Regular6pt7b);
        display_frame->println(payload.substring(payload.lastIndexOf("LFLY")));
      }
      https.end();

      https.begin(client, "https://api.met.no/weatherapi/tafmetar/1.0/taf?&date=" + date + "&icao=LFLY");

      code = https.GET();
      if (code == HTTP_CODE_OK)
      {
        String payload = https.getString();
        log_v("loop : %s", payload.substring(payload.lastIndexOf("LFLY")).c_str());
        display_frame->setFont(&Ubuntu_Bold7pt7b);
        display_frame->println("TAF");
        display_frame->setFont(&Ubuntu_Regular6pt7b);
        display_frame->println(payload);
      }
      https.end();

      xQueueSend(display_refresh_queue, &display_refresh, 0);

      vTaskDelay(600000 / portTICK_PERIOD_MS);
    }
  }
}
