#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_6
#define BTN GPIO_NUM_7

void app_main(void){
    // Led config
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Button config
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_config);

    while (1)
    {
        int pressed = (gpio_get_level(BTN) == 0); // true; false   1; 0

        gpio_set_level(LED_GPIO, pressed);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}
