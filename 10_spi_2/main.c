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

static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static int32_t t_fine;

// Read 1 byte
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
// ==================================================

// Read N bytes
static esp_err_t bmp280_read_bytes(uint8_t reg, uint8_t *out, size_t len){ // len 3
    uint8_t tx[len + 1];
    // MOSI 4
    // tx[0] = 0xFa | 0x80
    // tx[1] = 0x00 dummy
    // tx[2] = 0x00
    // tx[3] = 0x00
    uint8_t rx[len + 1];
    // MISO 4
    // rx[0] = null, garbage, dummy
    // rx[1] = from 0xFa
    // rx[2] = from 0xFb
    // rx[3] = from 0xFc
    memset(tx, 0, sizeof(tx)); // 0x00
    memset(rx, 0, sizeof(rx));

    tx[0] = reg | 0x80; // read bit

    spi_transaction_t t = {0};
    t.length = (len +1) *8;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t err = spi_device_transmit(dev, &t);
    if (err != ESP_OK) return err;

    memcpy(out, &rx[1], len);
    return ESP_OK;
}
// ==============================================

// write 1 byte
static esp_err_t bmp280_write_u8(uint8_t reg, uint8_t val){
    spi_transaction_t t = {0};
    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 16;
    t.tx_data[0] = reg & 0x7f; // write bit
    t.tx_data[1] = val;

    return spi_device_transmit(dev, &t);
}
// ============================================

// Read temperature calibration coeff......
static void bmp280_read_temp_calib(void){
    uint8_t b[6];
    ESP_ERROR_CHECK(bmp280_read_bytes(0x88, b, 6));

    dig_T1 = (uint16_t) (b[1] << 8 | b[0]);
    dig_T2 = (uint16_t) (b[3] << 8 | b[2]);
    dig_T3 = (uint16_t) (b[5] << 8 | b[4]);
}
// ============================================================

// Read temperature (20 bit)
static int32_t bmp280_read_temp(void){
    uint8_t d[3];
    ESP_ERROR_CHECK(bmp280_read_bytes(0xfa, d, 3));
    return (int32_t) ((d[0] << 12) | (d[1] << 4) | (d[2] >> 4)); // 20 bit
    // d0 [19:12]
    // d1 [11:4]
    // d2 [3:0]
}
// ==============================================================

// Bosch compensation formula
static int32_t bmp280_compensation(int32_t adc_T){
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
            ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}
// =======================================================

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

    bmp280_read_temp_calib();

    // Enable only temperature measurement
    ESP_ERROR_CHECK(bmp280_write_u8(0xf4, 0x23)); // 00100011

    while (1) {
        int32_t adc_T = bmp280_read_temp();
        int32_t T_01C = bmp280_compensation(adc_T);
        float tempC = T_01C / 100.0f;

        ESP_LOGI(TAG, " Temp= %.2f C", tempC);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
