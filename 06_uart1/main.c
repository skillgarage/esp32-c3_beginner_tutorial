#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "led_config.h"
#include "button_config.h"

#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 256

static const char *TAG = "UART_LEDBTN";

static void uart_init(void){
    // Set communication parameters
    uart_config_t cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1, 
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));
}

// Helper to send data
static void uart_send_str(const char *str){
    uart_write_bytes(UART_PORT, str, strlen(str));
}


void app_main(void)
{
    led_init();
    button_init();
    uart_init();

    int led_state = 0; // for led toggle
    int last_btn = gpio_get_level(BTN);

    while (1) {
        // Read bytes from Uart
        uint8_t ch = 0;
        int len = uart_read_bytes(UART_PORT, &ch, 1, pdMS_TO_TICKS(10));

        if (len > 0){
            switch (ch){
                case '1':
                    led_state = 1;
                    gpio_set_level(LED_GPIO, 1);
                    uart_send_str("LED: ON\r\n");
                    break;
                case '0':
                    led_state = 0;
                    gpio_set_level(LED_GPIO, 0);
                    uart_send_str("LED: OFF\r\n");
                    break;

                case 't':
                case 'T':
                    led_state = !led_state;
                    gpio_set_level(LED_GPIO, led_state);
                    uart_send_str("LED: TOGGLE\r\n");
                    break;
                    
                default:
                    break;
            }
        }

        int btn = gpio_get_level(BTN);

        if (btn != last_btn){
            last_btn = btn;
            if(btn == 0){
                uart_send_str("Button: PRESSED\r\n");
            } else {
                uart_send_str("Button: RELEASED\r\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
