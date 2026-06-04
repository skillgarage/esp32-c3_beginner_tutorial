#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_6
#define BTN GPIO_NUM_7
#define DEBOUNCE_MS 30
#define LONG_MS 800

void app_main(void){
    // Led config
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Button config
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));

    while (1){
        // check button pressed
        if (gpio_get_level(BTN) == 0){
            // get tick count
            TickType_t t0 = xTaskGetTickCount(); // 1 / 100 = 0.01 (10ms)  20000 ticks

            // debounce button pressed
            while(gpio_get_level(BTN) == 0 && (xTaskGetTickCount() - t0) < pdMS_TO_TICKS(DEBOUNCE_MS)){
                vTaskDelay(1);
            }

            if (gpio_get_level(BTN) == 0){
                t0 = xTaskGetTickCount(); // button pressed confirmed / 30000

                while (gpio_get_level(BTN) == 0) vTaskDelay(1);

                // debounce button released
                TickType_t now = xTaskGetTickCount();
                while(gpio_get_level(BTN) == 1 && (xTaskGetTickCount() - now) < pdMS_TO_TICKS(DEBOUNCE_MS)){
                    vTaskDelay(1);
                }

                TickType_t dur = xTaskGetTickCount() - t0; // 50000 - 30000 = 20000; 30050 - 30000

                if (dur >= pdMS_TO_TICKS(LONG_MS)){
                    gpio_set_level(LED_GPIO, 0);
                } else {
                    gpio_set_level(LED_GPIO, 1);
                }
            }
        }
        vTaskDelay(1);
    }
    
}
