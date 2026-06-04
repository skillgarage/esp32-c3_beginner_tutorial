#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t s_adc = NULL;

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

static int ldr_adc_read_avg_raw(void){
    int sum = 0;
    for(int i = 0; i < 32; i++){
        int raw = 0;
        adc_oneshot_read(s_adc, ADC_CHANNEL_1, &raw);
        sum += raw;
    }
    return sum / 32;
}

void app_main(void){
    ldr_adc_init();

    while (1) {
        int raw = ldr_adc_read_avg_raw();
        ESP_LOGI(TAG, "raw=%4d", raw);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
