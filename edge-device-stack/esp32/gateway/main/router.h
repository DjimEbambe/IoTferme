#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define ROUTER_MAX_ENTRIES 32
#define ROUTER_ASSET_ID_MAX 32

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t router_init(void);
esp_err_t router_register(const uint8_t mac[6], const char *asset_id, bool persistent);
const char *router_resolve(const uint8_t mac[6]);
bool router_lookup_asset(const char *asset_id, uint8_t mac_out[6]);
void router_touch(const uint8_t mac[6], int8_t rssi);
esp_err_t router_save(void);
void router_snapshot_json(char *buffer, size_t length);

typedef bool (*router_iter_cb_t)(const uint8_t mac[6], const char *asset_id, void *ctx);
void router_for_each(router_iter_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
