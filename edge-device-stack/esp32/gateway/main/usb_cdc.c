#include "usb_cdc.h"

#include <string.h>

#include "crc.h"
#include "esp_log.h"
#include "esp_tinyusb.h"
#include "tusb_cdc_acm.h"

static const char *TAG = "usb";
static usb_frame_cb_t s_handler = NULL;
static uint8_t s_rx_buffer[512];
static size_t s_rx_len = 0;

static size_t cobs_decode(const uint8_t *input, size_t length, uint8_t *output)
{
    size_t read_index = 0;
    size_t write_index = 0;
    while (read_index < length) {
        uint8_t code = input[read_index++];
        if (code == 0 || read_index + code - 1 > length) {
            return 0;
        }
        for (uint8_t i = 1; i < code; ++i) {
            output[write_index++] = input[read_index++];
        }
        if (code < 0xFF && read_index < length) {
            output[write_index++] = 0;
        }
    }
    return write_index;
}

static size_t cobs_encode(const uint8_t *input, size_t length, uint8_t *output)
{
    size_t read_index = 0;
    size_t write_index = 1;
    size_t code_index = 0;
    uint8_t code = 1;

    while (read_index < length) {
        if (input[read_index] == 0) {
            output[code_index] = code;
            code = 1;
            code_index = write_index++;
            read_index++;
        } else {
            output[write_index++] = input[read_index++];
            code++;
            if (code == 0xFF) {
                output[code_index] = code;
                code = 1;
                code_index = write_index++;
            }
        }
    }
    output[code_index] = code;
    output[write_index++] = 0;
    return write_index;
}

static void handle_frame(uint8_t *frame, size_t length)
{
    if (length < 2) {
        return;
    }
    uint16_t expected = ((uint16_t)frame[length - 2] << 8) | frame[length - 1];
    uint16_t actual = crc16_ccitt(frame, length - 2, 0xFFFF);
    if (expected != actual) {
        ESP_LOGW(TAG, "CRC mismatch" );
        return;
    }
    if (s_handler) {
        s_handler(frame, length - 2);
    }
}

static void usb_event_callback(int itf, tusb_cdcacm_event_t event, void *arg)
{
    if (event != TINYUSB_CDC_ACM_EVENT_RX) {
        return;
    }
    size_t available = 0;
    tinyusb_cdcacm_get_rx_buffer_size(itf, &available);
    if (available == 0) {
        return;
    }
    uint8_t chunk[64];
    size_t read = 0;
    tinyusb_cdcacm_read(itf, chunk, sizeof(chunk), &read);
    for (size_t i = 0; i < read; ++i) {
        if (chunk[i] == 0x00) {
            uint8_t decoded[512];
            size_t decoded_len = cobs_decode(s_rx_buffer, s_rx_len, decoded);
            if (decoded_len > 0) {
                handle_frame(decoded, decoded_len);
            }
            s_rx_len = 0;
        } else if (s_rx_len < sizeof(s_rx_buffer)) {
            s_rx_buffer[s_rx_len++] = chunk[i];
        }
    }
}

esp_err_t usb_cdc_init(usb_frame_cb_t handler)
{
    s_handler = handler;
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    ESP_RETURN_ON_ERROR(tinyusb_driver_install(&tusb_cfg), TAG, "tinyusb install");

    const tinyusb_config_cdcacm_t cdc_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 256,
        .callback_rx = usb_event_callback,
        .callback_tx = NULL,
        .callback_rx_wanted_char = NULL,
    };
    ESP_RETURN_ON_ERROR(tinyusb_cdcacm_init(&cdc_cfg), TAG, "cdc init");
    return ESP_OK;
}

esp_err_t usb_cdc_send(const uint8_t *data, size_t length)
{
    uint8_t frame[512];
    uint16_t crc = crc16_ccitt(data, length, 0xFFFF);
    uint8_t payload[512];
    if (length + 2 > sizeof(payload)) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(payload, data, length);
    payload[length] = (uint8_t)(crc >> 8);
    payload[length + 1] = (uint8_t)(crc & 0xFF);
    size_t encoded_len = cobs_encode(payload, length + 2, frame);
    size_t written = 0;
    esp_err_t rc = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, frame, encoded_len, &written);
    if (rc == ESP_OK) {
        rc = tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
    return rc;
}
