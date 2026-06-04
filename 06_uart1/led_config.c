#include "led_config.h"

void led_init(void){
    // Led config
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(LED_GPIO, 0);
}

void led_set(bool on){
    gpio_set_level(LED_GPIO, on ? 1 : 0);
}
