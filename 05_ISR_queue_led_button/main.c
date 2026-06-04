#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/queue.h"

#include "led_config.h"
#include "button_config.h"

#define DEBOUNCE_MS 30

static const char *TAG = "BTN_LED_ISR";

// Queue for button intr
static QueueHandle_t btn_evt_queue = NULL;

// ========  ISR (Interruption Service Routine) ========================
static void IRAM_ATTR btn_isr_handler(void *arg){ // Instruction RAM attribute
    uint32_t evt = 1; // Code event
    xQueueSendFromISR(btn_evt_queue, &evt, NULL);
}

void app_main(void){
    led_init();
    button_init();

    // Create button queue
    btn_evt_queue = xQueueCreate(5, sizeof(uint32_t));
    if (btn_evt_queue == NULL){
        return;
    }
    
    // install general service interruption
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    // add button interruption handler
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN, btn_isr_handler, NULL));

    // set led off
    bool led_on = false;
    led_set(led_on);

    const TickType_t DEB = pdMS_TO_TICKS(DEBOUNCE_MS);

    while(1){
        uint32_t evt = 0;

        if (xQueueReceive(btn_evt_queue, &evt, portMAX_DELAY)){
            TickType_t t0 = xTaskGetTickCount();

            while ((xTaskGetTickCount() - t0) < DEB){
                vTaskDelay(1);
            }

            if (gpio_get_level(BTN) != 0){
                ESP_LOGI(TAG, "Fake press");
            } else {
                while(gpio_get_level(BTN) == 0){
                    vTaskDelay(1);
                }
                t0 = xTaskGetTickCount();
                while((xTaskGetTickCount() - t0) < DEB){
                    vTaskDelay(1);
                }

                led_on = !led_on;
                led_set(led_on);
            }
            // clean queue
            while(xQueueReceive(btn_evt_queue, &evt, 0) == pdTRUE){

            }
    
        }
    }
}
