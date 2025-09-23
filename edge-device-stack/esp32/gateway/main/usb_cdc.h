#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*usb_frame_cb_t)(const uint8_t *data, size_t length);

esp_err_t usb_cdc_init(usb_frame_cb_t handler);
esp_err_t usb_cdc_send(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
