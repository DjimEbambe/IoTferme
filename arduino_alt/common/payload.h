#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"

namespace payload {

String make_uuid();
uint32_t crc32(const uint8_t *data, size_t length);

struct TelemetryEnv {
    String site = config::kSiteId;
    String device;
    String asset_id;
    float temp_c = NAN;
    float rh = NAN;
    float mq135_ppm = NAN;
    float lux = NAN;
    int16_t rssi_dbm = 0;
    bool alarm = false;
    String idempotency_key;
    String timestamp_iso;
};

struct TelemetryPower {
    String site = config::kSiteId;
    String device;
    String asset_id;
    float voltage = NAN;
    float current = NAN;
    float power_w = NAN;
    float energy_Wh = NAN;
    float pf = NAN;
    float frequency = NAN;
    int16_t rssi_dbm = 0;
    String idempotency_key;
    String timestamp_iso;
};

struct TelemetryWater {
    String site = config::kSiteId;
    String device;
    String asset_id;
    float level = NAN;
    float flow_l_min = NAN;
    bool pump_on = false;
    String idempotency_key;
    String timestamp_iso;
};

struct AckPayload {
    String asset_id;
    String correlation_id;
    bool ok = true;
    String message;
    String timestamp_iso;
};

struct RelaySetpoints {
    float t_c = NAN;
    float rh = NAN;
    float lux = NAN;
};

struct SequenceStep {
    String action;
    uint32_t duration_s = 0;
    uint32_t wait_s = 0;
};

struct RelayCommand {
    bool has_lamp = false;
    bool lamp_on = false;
    bool has_fan = false;
    bool fan_on = false;
    bool has_heater = false;
    bool heater_on = false;
    bool has_door = false;
    bool door_open = false;
    bool has_aux1 = false;
    bool aux1_on = false;
    bool has_aux2 = false;
    bool aux2_on = false;
};

struct CommandMessage {
    String asset_id;
    String correlation_id;
    RelayCommand relay;
    RelaySetpoints setpoints;
    SequenceStep sequence[6];
    size_t sequence_len = 0;
    bool pump_on_request = false;
    bool reset_energy = false;
};

size_t encode_telemetry_env(const TelemetryEnv &telemetry, uint8_t *out, size_t capacity);
size_t encode_telemetry_power(const TelemetryPower &telemetry, uint8_t *out, size_t capacity);
size_t encode_telemetry_water(const TelemetryWater &telemetry, uint8_t *out, size_t capacity);
size_t encode_ack(const AckPayload &ack, uint8_t *out, size_t capacity);

bool decode_command(const uint8_t *payload, size_t length, CommandMessage &out);

String iso_timestamp_now();

}  // namespace payload

