#include "frames.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "cJSON.h"

static const char *TAG = "frames";

static esp_err_t encode_json(cJSON *root, frame_buffer_t *frame)
{
    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        ESP_LOGE(TAG, "failed to serialise frame");
        return ESP_ERR_NO_MEM;
    }
    size_t length = strnlen(json, FRAME_BUFFER_MAX - 1);
    if (length >= FRAME_BUFFER_MAX) {
        cJSON_free(json);
        return ESP_ERR_NO_MEM;
    }
    memcpy(frame->buffer, json, length);
    frame->buffer[length] = '\0';
    frame->length = length + 1;
    cJSON_free(json);
    return ESP_OK;
}

static cJSON *telemetry_base(const char *site, const char *gateway_id, const char *asset_id, const char *channel)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddStringToObject(root, "site", site);
    cJSON_AddStringToObject(root, "device", gateway_id);
    cJSON_AddStringToObject(root, "asset_id", asset_id);
    cJSON_AddStringToObject(root, "channel", channel);
    return root;
}

static void telemetry_finalize(cJSON *root, float rssi_dbm, const char *fw, const char *msg_id)
{
    cJSON_AddNumberToObject(root, "rssi_dbm", rssi_dbm);
    if (fw) {
        cJSON_AddStringToObject(root, "fw", fw);
    }
    if (msg_id) {
        cJSON_AddStringToObject(root, "msg_id", msg_id);
    }
    cJSON_AddStringToObject(root, "ts", "1970-01-01T00:00:00Z");
}

esp_err_t frames_build_env_telemetry(
    frame_buffer_t *frame,
    const char *site,
    const char *gateway_id,
    const char *asset_id,
    float temperature_c,
    float humidity_pct,
    float mq135_ppm,
    float lux,
    int rssi_dbm,
    const char *fw,
    const char *msg_id)
{
    cJSON *root = telemetry_base(site, gateway_id, asset_id, "env");
    cJSON *metrics = cJSON_CreateObject();
    cJSON_AddNumberToObject(metrics, "t_c", temperature_c);
    cJSON_AddNumberToObject(metrics, "rh", humidity_pct);
    cJSON_AddNumberToObject(metrics, "mq135_ppm", mq135_ppm);
    cJSON_AddNumberToObject(metrics, "lux", lux);
    cJSON_AddItemToObject(root, "metrics", metrics);
    telemetry_finalize(root, rssi_dbm, fw, msg_id);
    esp_err_t rc = encode_json(root, frame);
    cJSON_Delete(root);
    return rc;
}

esp_err_t frames_build_power_telemetry(
    frame_buffer_t *frame,
    const char *site,
    const char *gateway_id,
    const char *asset_id,
    float voltage_v,
    float current_a,
    float power_w,
    float energy_wh,
    int rssi_dbm,
    const char *fw,
    const char *msg_id)
{
    cJSON *root = telemetry_base(site, gateway_id, asset_id, "power");
    cJSON *metrics = cJSON_CreateObject();
    cJSON_AddNumberToObject(metrics, "voltage_v", voltage_v);
    cJSON_AddNumberToObject(metrics, "current_a", current_a);
    cJSON_AddNumberToObject(metrics, "power_w", power_w);
    cJSON_AddNumberToObject(metrics, "energy_wh", energy_wh);
    cJSON_AddItemToObject(root, "metrics", metrics);
    telemetry_finalize(root, rssi_dbm, fw, msg_id);
    esp_err_t rc = encode_json(root, frame);
    cJSON_Delete(root);
    return rc;
}

esp_err_t frames_build_ack(
    frame_buffer_t *frame,
    const char *asset_id,
    const char *correlation_id,
    bool ok,
    const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "ack");
    cJSON_AddStringToObject(root, "asset_id", asset_id);
    cJSON_AddStringToObject(root, "correlation_id", correlation_id);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (message) {
        cJSON_AddStringToObject(root, "message", message);
    }
    cJSON_AddStringToObject(root, "ts", "1970-01-01T00:00:00Z");
    esp_err_t rc = encode_json(root, frame);
    cJSON_Delete(root);
    return rc;
}

esp_err_t frames_build_event(
    frame_buffer_t *frame,
    const char *asset_id,
    const char *event_type,
    const char *payload_json)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "asset_id", asset_id);
    cJSON_AddStringToObject(root, "event", event_type);
    if (payload_json) {
        cJSON *payload = cJSON_Parse(payload_json);
        if (payload) {
            cJSON_AddItemToObject(root, "payload", payload);
        } else {
            cJSON_AddStringToObject(root, "payload_raw", payload_json);
        }
    }
    cJSON_AddStringToObject(root, "ts", "1970-01-01T00:00:00Z");
    esp_err_t rc = encode_json(root, frame);
    cJSON_Delete(root);
    return rc;
}

esp_err_t frames_decode_command(const uint8_t *payload, size_t length, command_frame_t *out)
{
    memset(out, 0, sizeof(*out));
    char *buffer = strndup((const char *)payload, length);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return ESP_ERR_INVALID_ARG;
    }
    const cJSON *asset = cJSON_GetObjectItem(root, "asset_id");
    const cJSON *corr = cJSON_GetObjectItem(root, "correlation_id");
    if (!cJSON_IsString(asset) || !cJSON_IsString(corr)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }
    strlcpy(out->asset_id, asset->valuestring, sizeof(out->asset_id));
    strlcpy(out->correlation_id, corr->valuestring, sizeof(out->correlation_id));
    const cJSON *relay = cJSON_GetObjectItem(root, "relay");
    if (cJSON_IsObject(relay)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, relay) {
            if (out->relay_count >= 6) {
                break;
            }
            strlcpy(out->relay[out->relay_count].key, entry->string, sizeof(out->relay[0].key));
            strlcpy(out->relay[out->relay_count].value, cJSON_GetStringValue(entry), sizeof(out->relay[0].value));
            out->relay_count++;
        }
    }
    const cJSON *setpoints = cJSON_GetObjectItem(root, "setpoints");
    if (cJSON_IsObject(setpoints)) {
        const cJSON *t = cJSON_GetObjectItem(setpoints, "t_c");
        const cJSON *rh = cJSON_GetObjectItem(setpoints, "rh");
        const cJSON *lux = cJSON_GetObjectItem(setpoints, "lux");
        if (cJSON_IsNumber(t)) {
            out->setpoint_temp_c = (int32_t)(t->valuedouble * 100);
        }
        if (cJSON_IsNumber(rh)) {
            out->setpoint_rh = (int32_t)(rh->valuedouble * 100);
        }
        if (cJSON_IsNumber(lux)) {
            out->setpoint_lux = (int32_t)lux->valuedouble;
        }
    }
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t frames_extract_idempotency_key(const uint8_t *payload, size_t length, char *out, size_t out_len)
{
    char *buffer = strndup((const char *)payload, length);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return ESP_ERR_INVALID_ARG;
    }
    const cJSON *key = cJSON_GetObjectItem(root, "idempotency_key");
    if (cJSON_IsString(key)) {
        strlcpy(out, key->valuestring, out_len);
        cJSON_Delete(root);
        return ESP_OK;
    }
    cJSON_Delete(root);
    return ESP_ERR_NOT_FOUND;
}

void frames_make_msg_id(char *out, size_t out_len)
{
    static const char alphabet[] = "0123456789abcdef";
    for (size_t i = 0; i + 1 < out_len; ++i) {
        uint8_t value = (uint8_t)esp_random();
        out[i] = alphabet[value % (sizeof(alphabet) - 1)];
    }
    if (out_len > 0) {
        out[out_len - 1] = '\0';
    }
}
