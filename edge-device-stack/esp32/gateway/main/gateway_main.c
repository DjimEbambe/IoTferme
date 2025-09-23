#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ack_table.h"
#include "config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "frames.h"
#include "nvs_flash.h"
#include "provisioning.h"
#include "router.h"
#include "time_sync.h"
#include "usb_cdc.h"
#include "cJSON.h"
#include "espnow_init.h"

static const char *TAG = "gateway";

static void publish_ack(const char *asset_id, const char *corr, bool ok, const char *message)
{
    frame_buffer_t frame = {0};
    if (frames_build_ack(&frame, asset_id ? asset_id : "unknown", corr ? corr : "n/a", ok, message) == ESP_OK) {
        usb_cdc_send(frame.buffer, frame.length - 1);
    }
}

static void handle_command_frame(cJSON *root, const char *raw, size_t raw_len)
{
    const cJSON *asset = cJSON_GetObjectItem(root, "asset_id");
    const cJSON *corr = cJSON_GetObjectItem(root, "correlation_id");
    if (!cJSON_IsString(asset) || !cJSON_IsString(corr)) {
        ESP_LOGW(TAG, "command missing fields");
        return;
    }
    uint8_t mac[6];
    if (!router_lookup_asset(asset->valuestring, mac)) {
        publish_ack(asset->valuestring, corr->valuestring, false, "asset_not_paired");
        return;
    }
    ack_table_track(corr->valuestring);
    esp_err_t rc = espnow_send(mac, (const uint8_t *)raw, raw_len, true);
    if (rc != ESP_OK) {
        publish_ack(asset->valuestring, corr->valuestring, false, esp_err_to_name(rc));
    }
}

typedef struct {
    const uint8_t *payload;
    size_t length;
} broadcast_ctx_t;

static bool broadcast_cb(const uint8_t mac[6], const char *asset_id, void *ctx)
{
    (void)asset_id;
    const broadcast_ctx_t *bc = ctx;
    espnow_send(mac, bc->payload, bc->length, true);
    return true;
}

static void broadcast_time_sync(const uint8_t *payload, size_t len)
{
    broadcast_ctx_t ctx = {
        .payload = payload,
        .length = len,
    };
    router_for_each(broadcast_cb, &ctx);
}

static void handle_usb_frame(const uint8_t *data, size_t len)
{
    char *buffer = strndup((const char *)data, len);
    if (!buffer) {
        return;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        ESP_LOGW(TAG, "invalid JSON from USB");
        return;
    }
    const cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) {
        handle_command_frame(root, (const char *)data, len);
        cJSON_Delete(root);
        return;
    }
    if (strcmp(type->valuestring, "cmd") == 0) {
        handle_command_frame(root, (const char *)data, len);
    } else if (strcmp(type->valuestring, "pair_begin") == 0) {
        uint32_t duration = 120;
        const cJSON *dur = cJSON_GetObjectItem(root, "duration_s");
        if (cJSON_IsNumber(dur)) {
            duration = (uint32_t)dur->valuedouble;
        }
        provisioning_begin(duration);
    } else if (strcmp(type->valuestring, "pair_end") == 0) {
        provisioning_end();
    } else if (strcmp(type->valuestring, "time_sync") == 0) {
        time_sync_handle_frame(data, len);
        broadcast_time_sync(data, len);
    } else if (strcmp(type->valuestring, "ping") == 0) {
        const cJSON *asset = cJSON_GetObjectItem(root, "asset_id");
        if (cJSON_IsString(asset)) {
            uint8_t mac[6];
            if (router_lookup_asset(asset->valuestring, mac)) {
                espnow_send(mac, data, len, true);
            }
        }
    }
    cJSON_Delete(root);
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    int8_t rssi = 0;
    if (info->rx_ctrl != NULL) {
        rssi = info->rx_ctrl->rssi;
    }
    router_touch(info->src_addr, rssi);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    char *buffer = strndup((const char *)data, len);
    if (!buffer) {
        return;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        ESP_LOGW(TAG, "invalid JSON from device");
        return;
    }
    cJSON_AddStringToObject(root, "mac", mac_str);
    cJSON_AddNumberToObject(root, "rssi_dbm", rssi);
    const cJSON *asset = cJSON_GetObjectItem(root, "asset_id");
    if (cJSON_IsString(asset)) {
        if (!router_resolve(info->src_addr)) {
            router_register(info->src_addr, asset->valuestring, true);
        }
    }
    const cJSON *type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsString(type) && strcmp(type->valuestring, "ack") == 0) {
        const cJSON *corr = cJSON_GetObjectItem(root, "correlation_id");
        if (cJSON_IsString(corr)) {
            ack_table_clear(corr->valuestring);
        }
    }
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        usb_cdc_send((const uint8_t *)json, strlen(json));
        cJSON_free(json);
    }
    cJSON_Delete(root);
}

static void init_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(EDGE_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(router_init());
    ack_table_init();
    provisioning_init();
    time_sync_init();

    init_wifi();
    uint8_t pmk[ESP_NOW_KEY_LEN] = {0};
    memcpy(pmk, EDGE_ESPNOW_PMK, strnlen(EDGE_ESPNOW_PMK, ESP_NOW_KEY_LEN));
    uint8_t lmk[ESP_NOW_KEY_LEN] = {0};
    memcpy(lmk, EDGE_ESPNOW_LMK, strnlen(EDGE_ESPNOW_LMK, ESP_NOW_KEY_LEN));
    ESP_ERROR_CHECK(espnow_setup(pmk, lmk, EDGE_ESPNOW_CHANNEL));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    ESP_ERROR_CHECK(usb_cdc_init(handle_usb_frame));
    ESP_LOGI(TAG, "gateway initialised");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
