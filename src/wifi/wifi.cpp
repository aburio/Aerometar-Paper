#include "wifi.h"

#include <WiFi.h>

/* local types & variables */
QueueHandle_t wifi_event_queue;

/* private functions prototypes */

/* public functions */
/**
 * @brief Initialize the wifi connectivity
 * 
 * @param ssid wifi ssid to connect
 * @param password wifi password to connect
 */
void wifiInit(const String ssid, const String password)
{
    // init variables
    wifi_event_queue = xQueueCreate(1, sizeof(int8_t));

    // setup wifi
    WiFi.mode(WIFI_MODE_STA);
    WiFi.softAP("Aerometar-Paper");
    WiFi.setHostname("Aerometar-Paper");
    WiFi.setAutoConnect(false);
    WiFi.begin(ssid.c_str(), password.c_str());
    log_v("wifi_init : init done");
}

/**
 * @brief Task to control the wifi connectivity
 * 
 * @param pvParameters 
 */
void wifiTask(void *pvParameters)
{
    bool wifi_connected = false;
    wl_status_t wifi_status = WiFi.status();

    log_v("wifi_task : start task");
    xQueueSend(wifi_event_queue, &wifi_status, 0);

    if (wifi_status == WL_IDLE_STATUS)
    {
        WiFi.begin();
    }

    for(;;)
    {
        wifi_status = WiFi.status();

        switch (wifi_status)
        {
        case WL_CONNECTED:
            if (wifi_connected == false)
            {
                log_v("wifi_task : connected");
                wifi_connected = true;
                xQueueSend(wifi_event_queue, &wifi_status, 0);
            }
            break;
        
        case WL_NO_SSID_AVAIL:
        case WL_CONNECT_FAILED:
            log_v("wifi_task : access point");
            WiFi.mode(WIFI_MODE_AP);
            WiFi.softAP("Aerometar-Paper");
            xQueueSend(wifi_event_queue, &wifi_status, 0);
            break;

        case WL_CONNECTION_LOST:
            log_v("wifi_task : reconnect");
            WiFi.reconnect();
            xQueueSend(wifi_event_queue, &wifi_status, 0);
            break;
        
        case WL_IDLE_STATUS:
        case WL_DISCONNECTED:
        default:
            log_v("wifi_task : connecting");
            if (wifi_connected == true)
            {
                wifi_connected = false;
                xQueueSend(wifi_event_queue, &wifi_status, 0);
            }
            break;
        }

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);
}

/* private functions */