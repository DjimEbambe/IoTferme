#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t espnow_setup(const uint8_t *pmk, const uint8_t *lmk, int channel);
esp_err_t espnow_add_peer(const uint8_t *mac);
esp_err_t espnow_remove_peer(const uint8_t *mac);
esp_err_t espnow_send(const uint8_t *mac, const uint8_t *data, size_t len, bool encrypt);
void espnow_register_recv(esp_now_recv_cb_t cb);

#ifdef __cplusplus
}
#endif
