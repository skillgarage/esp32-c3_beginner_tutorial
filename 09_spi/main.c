#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"

static const char *TAG = "SPI";

#define PIN_CS 10
#define PIN_SCLK 6
#define PIN_MISO 2
#define PIN_MOSI 7

#define REG_ID 0xD0
#define BMP280_ID 0x58

static spi_device_handle_t dev = NULL;

static esp_err_t bmp280_read(uint8_t reg, uint8_t *val){
    uint8_t tx[2] = {(uint8_t) (reg | 0x80), 0x00}; // 0x80 = 1000 0000 MOSI
    uint8_t rx[2] = {0x00, 0x00}; // MISO rx[0], rx[1] = 0x58

    spi_transaction_t t = {0};
    t.length = 16;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t err = spi_device_transmit(dev, &t);
    if (err != ESP_OK) return err;

    *val = rx[1]; 
    return ESP_OK;
}

void app_main(void){
    // spi bus init
    spi_bus_config_t busconfig = {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_SCLK,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &busconfig, SPI_DMA_CH_AUTO));
    // ==============================================================================
    
    // add device (bmp280)
    spi_device_interface_config_t devconfig = {
        .mode = 0,
        .clock_speed_hz = 1 * 1000 *1000,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devconfig, &dev));
    // =======================================================================================

    uint8_t id = 0;
    ESP_ERROR_CHECK(bmp280_read(REG_ID, &id));

    while (1) {
        if (id == BMP280_ID){
            ESP_LOGI(TAG, "Detected bmp280, address 0x58, SPI OK");
        } else {
            ESP_LOGI(TAG, "Error");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
