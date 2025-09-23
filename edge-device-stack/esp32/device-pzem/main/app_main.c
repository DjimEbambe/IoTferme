#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "config.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_store.h"
#include "nvs_flash.h"
#include "pzem.h"
#include "relays.h"
#include "time_sync.h"
#include "espnow_init.h"
#include "frames.h"

static const char *TAG = "pzem_device";
static uint8_t s_gateway_mac[ESP_NOW_ETH_ALEN] = {0};
static const uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_gateway_known = false;

static void iso_timestamp(char *buffer, size_t len)
{
    int64_t epoch_ms = time_sync_now_ms();
    time_t seconds = epoch_ms / 1000;
    struct tm tm_utc = {0};
    gmtime_r(&seconds, &tm_utc);
    int ms = (int)(epoch_ms % 1000);
    snprintf(buffer, len, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
             tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec, ms);
}

static esp_err_t send_to_gateway(const uint8_t *payload, size_t len)
{
    bool encrypt = s_gateway_known;
    const uint8_t *target = s_gateway_known ? s_gateway_mac : s_broadcast_mac;
    return espnow_send(target, payload, len, encrypt);
}

static void publish_ack(const char *corr, bool ok, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "ack");
    cJSON_AddStringToObject(root, "asset_id", "PV-INV-01");
    cJSON_AddStringToObject(root, "correlation_id", corr ? corr : "n/a");
    cJSON_AddBoolToObject(root, "ok", ok);
    if (message) {
        cJSON_AddStringToObject(root, "message", message);
    }
    char ts[32];
    iso_timestamp(ts, sizeof(ts));
    cJSON_AddStringToObject(root, "ts", ts);
    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        send_to_gateway((const uint8_t *)json, strlen(json));
        cJSON_free(json);
    }
    cJSON_Delete(root);
}

static void handle_command(const uint8_t *data, size_t len)
{
    char *buffer = strndup((const char *)data, len);
    if (!buffer) {
        return;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return;
    }
    const cJSON *corr = cJSON_GetObjectItem(root, "correlation_id");
    const cJSON *relay = cJSON_GetObjectItem(root, "relay");
    if (cJSON_IsObject(relay)) {
        const cJSON *pump = cJSON_GetObjectItem(relay, "pump");
        if (cJSON_IsString(pump)) {
            bool on = strcasecmp(pump->valuestring, "ON") == 0;
            pzem_relays_set(RELAY_PUMP, on);
        }
    }
    publish_ack(cJSON_IsString(corr) ? corr->valuestring : "n/a", true, "applied");
    cJSON_Delete(root);
}

static void handle_control_message(const uint8_t *data, size_t len)
{
    char *buffer = strndup((const char *)data, len);
    if (!buffer) {
        return;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return;
    }
    const cJSON *type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsString(type) && strcmp(type->valuestring, "cmd") == 0) {
        handle_command(data, len);
    } else if (cJSON_IsString(type) && strcmp(type->valuestring, "time_sync") == 0) {
        time_sync_handle_frame(data, len);
    } else if (cJSON_IsString(type) && strcmp(type->valuestring, "pair_ack") == 0) {
        const cJSON *mac = cJSON_GetObjectItem(root, "gateway_mac");
        if (cJSON_IsString(mac) && strlen(mac->valuestring) == 17) {
            sscanf(mac->valuestring, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &s_gateway_mac[0], &s_gateway_mac[1], &s_gateway_mac[2],
                   &s_gateway_mac[3], &s_gateway_mac[4], &s_gateway_mac[5]);
            s_gateway_known = true;
        }
    }
    cJSON_Delete(root);
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    handle_control_message(data, len);
}

static void telemetry_task(void *arg)
{
    (void)arg;
    const TickType_t interval = pdMS_TO_TICKS(EDGE_POWER_INTERVAL_MS);
    TickType_t last_wake = xTaskGetTickCount();
    pzem_sample_t sample = {0};
    while (true) {
        if (pzem_read(&sample) == ESP_OK) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "telemetry");
            cJSON_AddStringToObject(root, "site", EDGE_SITE_ID);
            cJSON_AddStringToObject(root, "device", EDGE_DEVICE_ID);
            cJSON_AddStringToObject(root, "asset_id", EDGE_DEVICE_ASSET_POWER);
            cJSON_AddStringToObject(root, "channel", "power");
            cJSON *metrics = cJSON_CreateObject();
            cJSON_AddNumberToObject(metrics, "voltage_v", sample.voltage_v);
            cJSON_AddNumberToObject(metrics, "current_a", sample.current_a);
            cJSON_AddNumberToObject(metrics, "power_w", sample.power_w);
            cJSON_AddNumberToObject(metrics, "energy_wh", sample.energy_wh);
            cJSON_AddItemToObject(root, "metrics", metrics);
            char ts[32];
            iso_timestamp(ts, sizeof(ts));
            cJSON_AddStringToObject(root, "ts", ts);
            char msg_id[EDGE_MSG_ID_LENGTH] = {0};
            frames_make_msg_id(msg_id, sizeof(msg_id));
            cJSON_AddStringToObject(root, "msg_id", msg_id);
            cJSON_AddStringToObject(root, "idempotency_key", msg_id);
            char *json = cJSON_PrintUnformatted(root);
            if (json) {
                send_to_gateway((const uint8_t *)json, strlen(json));
                cJSON_free(json);
            }
            cJSON_Delete(root);
        }
        vTaskDelayUntil(&last_wake, interval);
    }
}

static void init_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(EDGE_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    nvs_store_init("pzem");
    time_sync_init();
    pzem_relays_init();
    pzem_relays_off();
    pzem_init(0x01);

    init_wifi();
    uint8_t pmk[ESP_NOW_KEY_LEN] = {0};
    memcpy(pmk, EDGE_ESPNOW_PMK, strnlen(EDGE_ESPNOW_PMK, ESP_NOW_KEY_LEN));
    uint8_t lmk[ESP_NOW_KEY_LEN] = {0};
    memcpy(lmk, EDGE_ESPNOW_LMK, strnlen(EDGE_ESPNOW_LMK, ESP_NOW_KEY_LEN));
    ESP_ERROR_CHECK(espnow_setup(pmk, lmk, EDGE_ESPNOW_CHANNEL));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    xTaskCreate(telemetry_task, "pzem", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "device-pzem started");
}
