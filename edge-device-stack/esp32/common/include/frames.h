#pragma once

/**
 * @file frames.h
 * @brief Helpers to build and parse CBOR/JSON frames exchanged with the edge agent.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_BUFFER_MAX 512

typedef struct {
    char asset_id[24];
    char correlation_id[40];
    struct {
        char key[16];
        char value[8];
    } relay[6];
    size_t relay_count;
    int32_t setpoint_temp_c;
    int32_t setpoint_rh;
    int32_t setpoint_lux;
} command_frame_t;

/**
 * @brief Container for encoded payload.
 */
typedef struct {
    uint8_t buffer[FRAME_BUFFER_MAX];
    size_t length;
} frame_buffer_t;

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
    const char *msg_id
);

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
    const char *msg_id
);

esp_err_t frames_build_ack(
    frame_buffer_t *frame,
    const char *asset_id,
    const char *correlation_id,
    bool ok,
    const char *message
);

esp_err_t frames_build_event(
    frame_buffer_t *frame,
    const char *asset_id,
    const char *event_type,
    const char *payload_json
);

esp_err_t frames_decode_command(const uint8_t *payload, size_t length, command_frame_t *out);

esp_err_t frames_extract_idempotency_key(const uint8_t *payload, size_t length, char *out, size_t out_len);

void frames_make_msg_id(char *out, size_t out_len);

#ifdef __cplusplus
}
#endif
