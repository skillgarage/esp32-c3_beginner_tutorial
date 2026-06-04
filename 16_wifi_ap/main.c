#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"

#define LED_GPIO 7

///////  HTTP Handlers  /////////////////
static esp_err_t root_handler(httpd_req_t *req){
    const char* resp = 
        "<h1>ESP32C3 Led control<h1>"
        "<a href=\"/on\">ON</a><br>"
        "<a href=\"/off\">OFF</a>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t on_handler(httpd_req_t *req){
    gpio_set_level(LED_GPIO, 1);
    httpd_resp_send(req, "LED ON", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t off_handler(httpd_req_t *req){
    gpio_set_level(LED_GPIO, 0);
    httpd_resp_send(req, "LED OFF", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
///////////////////////////////////////

///////  Start Web Server  /////////////////////////////
static httpd_handle_t start_webserver(void){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK) {
            httpd_uri_t root = {
                .uri = "/",     // 192.168.4.1/
                .method = HTTP_ANY,
                .handler = root_handler
            };
            httpd_register_uri_handler(server, &root);

            httpd_uri_t on = {
                .uri = "/on",     // 192.168.4.1/on
                .method = HTTP_ANY,
                .handler = on_handler
            };
            httpd_register_uri_handler(server, &on);

            httpd_uri_t off = {
                .uri = "/off",     // 192.168.4.1/off
                .method = HTTP_ANY,
                .handler = off_handler
            };
            httpd_register_uri_handler(server, &off);

        }
    return server;
}
// ==========================================================

////  Wifi AP //////////////////////
static void wifi_init_ap(void){
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32C3-AP",
            .password = "12345678",
            .channel = 1,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}
// ===================================

void app_main(void){
    nvs_flash_init();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    wifi_init_ap();
    start_webserver(); //  196.168.4.1/
}
