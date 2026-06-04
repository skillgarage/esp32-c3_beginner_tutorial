#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"

static const char *TAG = "ADC_PWM";

static adc_oneshot_unit_handle_t s_adc = NULL;

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

void app_main(void){
    ldr_adc_init();
    ledc_init_simple();
    // uint32_t duty_res = ledc_find_suitable_duty_resolution(80000000, 5000);
    while (1) {
        int raw = ldr_adc_read_avg_raw();
        led_set_brightness_from_raw(raw);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
