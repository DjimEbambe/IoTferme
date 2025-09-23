#include "pzem.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pins.h"

#define PZEM_UART_BAUD 9600

static const char *TAG = "pzem";
static uint8_t s_address = 0x01;
static bool s_ready = false;

static uint16_t crc16_modbus(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

esp_err_t pzem_init(uint8_t address)
{
    if (s_ready) {
        return ESP_OK;
    }
    s_address = address;
    uart_config_t cfg = {
        .baud_rate = PZEM_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_driver_install(EDGE_RS485_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(EDGE_RS485_UART_NUM, &cfg));
    uart_set_pin(EDGE_RS485_UART_NUM, EDGE_RS485_TX_GPIO, EDGE_RS485_RX_GPIO, EDGE_RS485_DE_GPIO, EDGE_RS485_RE_GPIO);
    gpio_set_direction(EDGE_RS485_DE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(EDGE_RS485_RE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(EDGE_RS485_DE_GPIO, 0);
    gpio_set_level(EDGE_RS485_RE_GPIO, 0);
    s_ready = true;
    ESP_LOGI(TAG, "init addr=0x%02x", s_address);
    return ESP_OK;
}

static esp_err_t modbus_request(const uint8_t *request, size_t req_len, uint8_t *response, size_t resp_len)
{
    gpio_set_level(EDGE_RS485_DE_GPIO, 1);
    gpio_set_level(EDGE_RS485_RE_GPIO, 1);
    uart_write_bytes(EDGE_RS485_UART_NUM, (const char *)request, req_len);
    uart_wait_tx_done(EDGE_RS485_UART_NUM, pdMS_TO_TICKS(20));
    gpio_set_level(EDGE_RS485_DE_GPIO, 0);
    gpio_set_level(EDGE_RS485_RE_GPIO, 0);
    int read = uart_read_bytes(EDGE_RS485_UART_NUM, response, resp_len, pdMS_TO_TICKS(200));
    if (read != (int)resp_len) {
        ESP_LOGW(TAG, "response timeout %d", read);
        return ESP_FAIL;
    }
    uint16_t crc = crc16_modbus(response, resp_len - 2);
    uint16_t expected = response[resp_len - 2] | (response[resp_len - 1] << 8);
    if (crc != expected) {
        ESP_LOGW(TAG, "crc mismatch");
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t pzem_read(pzem_sample_t *sample)
{
    if (!s_ready) {
        ESP_RETURN_ON_ERROR(pzem_init(0x01), TAG, "init");
    }
    uint8_t request[8] = {s_address, 0x04, 0x00, 0x00, 0x00, 0x0A};
    uint16_t crc = crc16_modbus(request, 6);
    request[6] = crc & 0xFF;
    request[7] = crc >> 8;

    uint8_t response[25] = {0};
    ESP_RETURN_ON_ERROR(modbus_request(request, sizeof(request), response, sizeof(response)), TAG, "query");

    float voltage = ((response[3] << 8) | response[4]) / 10.0f;
    float current = ((response[5] << 8) | response[6]) / 100.0f;
    float power = ((response[7] << 8) | response[8]) / 10.0f;
    float energy = ((response[9] << 8) | response[10]) * 1.0f;  // Wh
    sample->voltage_v = voltage;
    sample->current_a = current;
    sample->power_w = power;
    sample->energy_wh = energy;
    return ESP_OK;
}
