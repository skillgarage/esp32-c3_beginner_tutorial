// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_log.h"


// #include "led_config.h"
// #include "button_config.h"
// #include "uart_config.h"

// void app_main() {
//     uart_init();
//     led_init();

//     uint8_t ch;

//     while (1) {
//         int len = uart_read_bytes(UART_PORT, &ch, 1, portMAX_DELAY);
//         if (len > 0){
//             if (ch == '1'){
//                 gpio_set_level(LED_GPIO, 1);
//             } else if (ch == '0'){
//                 gpio_set_level(LED_GPIO, 0);
//             }
//         }
//     }
// }

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"


#include "led_config.h"
#include "button_config.h"
#include "uart_config.h"

static const char *TAG = "UART_MASTER";

void app_main() {
    uart_init();
    button_init();

    bool led_state = false;
    int last_btn = 1;

    while (1) {
        int now = gpio_get_level(BTN); // on pressed become 0, else 1

        if (last_btn == 1 && now == 0){
            vTaskDelay(pdMS_TO_TICKS(30));
            if (gpio_get_level(BTN) == 0){
                led_state = !led_state;  // toggle
                char ch = led_state ? '1' : '0';
                uart_write_bytes(UART_PORT, &ch, 1);
                ESP_LOGI(TAG, "Button pressed, send '%c", ch);
            }
        }
        last_btn = now;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
