#include "sht41.h"

#include <string.h>

#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pins.h"

#define SHT41_ADDR 0x44

static const char *TAG = "sht41";
static bool s_initialised = false;

static esp_err_t i2c_master_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EDGE_I2C_SDA_GPIO,
        .scl_io_num = EDGE_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM_0, &cfg), TAG, "param");
    return i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0);
}

static esp_err_t sht41_send_command(uint16_t command)
{
    uint8_t buffer[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    return i2c_master_write_to_device(I2C_NUM_0, SHT41_ADDR, buffer, sizeof(buffer), pdMS_TO_TICKS(20));
}

static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

esp_err_t sht41_init(void)
{
    if (s_initialised) {
        return ESP_OK;
    }
    ESP_ERROR_CHECK(i2c_master_init());
    s_initialised = true;
    ESP_LOGI(TAG, "initialised (I2C %d)", I2C_NUM_0);
    return ESP_OK;
}

esp_err_t sht41_read(sht41_sample_t *out_sample)
{
    if (!s_initialised) {
        ESP_RETURN_ON_ERROR(sht41_init(), TAG, "init");
    }
    ESP_RETURN_ON_ERROR(sht41_send_command(0xFD), TAG, "measure");
    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t buffer[6] = {0};
    ESP_RETURN_ON_ERROR(i2c_master_read_from_device(I2C_NUM_0, SHT41_ADDR, buffer, sizeof(buffer), pdMS_TO_TICKS(20)), TAG, "read");
    if (crc8(buffer, 2) != buffer[2] || crc8(&buffer[3], 2) != buffer[5]) {
        return ESP_ERR_INVALID_CRC;
    }
    uint16_t raw_t = ((uint16_t)buffer[0] << 8) | buffer[1];
    uint16_t raw_rh = ((uint16_t)buffer[3] << 8) | buffer[4];
    out_sample->temperature_c = -45.0f + 175.0f * (float)raw_t / 65535.0f;
    out_sample->humidity_pct = -6.0f + 125.0f * (float)raw_rh / 65535.0f;
    if (out_sample->humidity_pct < 0) {
        out_sample->humidity_pct = 0;
    } else if (out_sample->humidity_pct > 100) {
        out_sample->humidity_pct = 100;
    }
    return ESP_OK;
}
