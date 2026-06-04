#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"


static const char *TAG = "WIFI_STA";

#define MAX_RETRY 8
static int s_retry = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    (void)arg; (void)event_base; (void)event_data;

    switch(event_id){
        case WIFI_EVENT_STA_START:
            s_retry = 0;
            ESP_LOGI(TAG, "Connecting...");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            if(s_retry < MAX_RETRY){
                s_retry++;
                ESP_ERROR_CHECK(esp_wifi_connect());
            } else {
                ESP_LOGE(TAG, "Failed to connect, weak signal, wrong password.....");
            }
            break;
        default:
            break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    (void)arg; (void)event_base; (void)event_data;

    if(event_id == IP_EVENT_STA_GOT_IP){
        s_retry = 0;
        ESP_LOGI(TAG, "Connected, ip received.");
    }
}

void app_main() {
    vTaskDelay(pdMS_TO_TICKS(5000));

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Starting wifi...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // NVS init Non-volatile storage
    ESP_ERROR_CHECK(nvs_flash_init());

    // Netif init
    ESP_ERROR_CHECK(esp_netif_init()); // drivers; TCR/IP core

    // Enent loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Netif create STA
    esp_netif_create_default_wifi_sta();

    // Wifi driver init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Subscribe to events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)); // WIFI_EVENT_*
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    // Wifi credentials
    wifi_config_t w = {0};
    strncpy((char*)w.sta.ssid, WIFI_SSID, sizeof(w.sta.ssid)); // max 32 bytes
    strncpy((char*)w.sta.password, WIFI_PASS, sizeof(w.sta.password)); // max 64 bytes

    // Set STA mode 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Aplly config to STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w));

    // Start wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wifi started.");
}
