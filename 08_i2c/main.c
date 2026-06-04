#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

static const char *TAG = "I2C";

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_PIN     GPIO_NUM_8
#define I2C_SCL_PIN     GPIO_NUM_9
#define I2C_FREQ_HZ     100000     

static i2c_master_bus_handle_t i2c_bus = NULL;

// I2c init master bus
static void i2c_master_bus_init(void){
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
            .allow_pd = false,
        },
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));
}
// ==========================================================================

static int i2c_scan_bus_new(void){
    int found_addr = -1;

    for (uint8_t addr = 1; addr < 127; addr++){
        esp_err_t err = i2c_master_probe(i2c_bus, addr, 50);

        if(err == ESP_OK){
            found_addr = addr;
            break;
        }
    }

    return found_addr;
}

void app_main(void){
    i2c_master_bus_init();
    vTaskDelay(pdMS_TO_TICKS(50));

    int dev_addr = i2c_scan_bus_new();

    while(1){
        if(dev_addr >= 0){
            ESP_LOGI(TAG, "Device address = %d (0x%02X)", dev_addr, dev_addr);
        } else {
            ESP_LOGI(TAG, "Device not found");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
