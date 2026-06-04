#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_attr.h"

#define WAKE_BTN_GPIO 4
#define LIGHT_SLEEP_US (2ULL * 1000000ULL) // 2sec
#define ACTIVE_MS 5000 // 5 sec
#define LIGHT_CYCLES_BEFORE_DEEP 3

static const char *TAG = "ADC_PWM_SLEEP";

static adc_oneshot_unit_handle_t s_adc = NULL;

// value for deep sleep
RTC_DATA_ATTR static uint32_t boot_count = 0;
// normal value
static uint32_t runtime_counter = 0;

// ======= adc init ==========================
static void ldr_adc_init(void){
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, ADC_CHANNEL_1, &chan_cfg));
}
// ==============================================

// ===== read raw ==============================
static int ldr_adc_read_avg_raw(void){
    int sum = 0;
    for(int i = 0; i < 32; i++){
        int raw = 0;
        adc_oneshot_read(s_adc, ADC_CHANNEL_1, &raw);
        sum += raw;
    }
    return sum / 32;
}
// ==================================================

// ====== pwm (ledc) init ==========================
static void ledc_init_simple(void){
    ledc_timer_config_t timer_cfg = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = LEDC_TIMER_13_BIT, // from 0 to 8191 -> 0
      .freq_hz = 5000, // 5000 periods pwm in 1 sec
      .clk_cfg = LEDC_AUTO_CLK  
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num = 3,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0 // LPoint = hpoint + duty
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
}
// ===========================================

static void led_set_brightness_from_raw(int raw_0_4095){
    const uint32_t max_duty = (1U << 13) - 1; // 8191
    uint32_t duty = ((uint32_t)raw_0_4095 * max_duty) / 4095; // raw = 0 -> duty = 0; raw = 2048 -> duty = 4096; raw 4095 -> duty = 8191

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0)); // raw = 4000 -> duty = 8000; ledOn; timer > duty ledOff
    // 800 / 8191 = 9%
    // 8000 / 8191 = 97%
}

// convert to readable formsat
static const char* wakeup_cause_to_str(esp_sleep_wakeup_cause_t cause){
    switch(cause){
        case ESP_SLEEP_WAKEUP_TIMER: return "TIMER";
        case ESP_SLEEP_WAKEUP_GPIO: return "GPIO";
        default: return "OTHER";
    }
}
// ====================================================

// button init for deep wakeup
static void wake_button_init(void){
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << WAKE_BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}
// ====================================================

// debounce 
static void debounce_after_deep_wakeup(void){
    vTaskDelay(pdMS_TO_TICKS(30));
    while(gpio_get_level(WAKE_BTN_GPIO) == 0){
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// ===============================================

void app_main(void){
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    boot_count++;
    ESP_LOGI(TAG, "Wakeup cause: %s", wakeup_cause_to_str(cause));
    ESP_LOGI(TAG, "boot_count (RTC) = %lu, runtime_counter (normal) = %lu",
        (unsigned long)boot_count, (unsigned long)runtime_counter);

    wake_button_init();
    if(cause == ESP_SLEEP_WAKEUP_GPIO){
        debounce_after_deep_wakeup();
    }

    ldr_adc_init();
    ledc_init_simple();
    // uint32_t duty_res = ledc_find_suitable_duty_resolution(80000000, 5000);

    int light_cycles = 0;

    while (1) {
        // after N cycles of active operations going to deep sleep
        if(light_cycles >= LIGHT_CYCLES_BEFORE_DEEP){
            ESP_LOGI(TAG, "Enter deep sleep now.");
            ESP_LOGI(TAG, "Wakeup: press button: GPIO%d", WAKE_BTN_GPIO);

            ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER));
            ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_BTN_GPIO, ESP_GPIO_WAKEUP_GPIO_LOW));

            vTaskDelay(pdMS_TO_TICKS(50));
            esp_deep_sleep_start();
        }
        // =======================================================================

        // Active operation: 5 sec
        TickType_t t0 = xTaskGetTickCount();
        while((xTaskGetTickCount() - t0) < pdMS_TO_TICKS(ACTIVE_MS)){
            int raw = ldr_adc_read_avg_raw();
            led_set_brightness_from_raw(raw);

            runtime_counter++;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        ESP_LOGI(TAG, "Cycle done. runtime_counter = %lu", (unsigned long)runtime_counter);
        // ===================================================

        // light sleep
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_US));
        ESP_LOGI(TAG, "Enter light sleep for %llu ms (cycle %d/%d)", (unsigned long long)(LIGHT_SLEEP_US / 1000ULL),
            light_cycles + 1, LIGHT_CYCLES_BEFORE_DEEP);

        esp_light_sleep_start();

        esp_sleep_wakeup_cause_t c = esp_sleep_get_wakeup_cause();
        ESP_LOGI(TAG, "Wake form light sleep, cause %s", wakeup_cause_to_str(c));

        light_cycles++;
    }
}
