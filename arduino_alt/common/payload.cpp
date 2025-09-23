#include "payload.h"

#include <WiFi.h>
#include <esp_random.h>
#include <time.h>

#include "debug_log.h"

namespace payload {

namespace {

size_t serialize(const JsonDocument &doc, uint8_t *out, size_t capacity) {
    if (!out || capacity == 0) {
        return 0;
    }
#if USE_MSGPACK
    return serializeMsgPack(doc, out, capacity);
#else
    return serializeJson(doc, out, capacity);
#endif
}

bool deserialize(const uint8_t *payload, size_t length, JsonDocument &doc) {
#if USE_MSGPACK
    DeserializationError err = deserializeMsgPack(doc, payload, length);
#else
    DeserializationError err = deserializeJson(doc, payload, length);
#endif
    if (err) {
        LOGW("payload deserialization failed: %s", err.c_str());
        return false;
    }
    return true;
}

void ensure_timestamp(const String &provided, JsonDocument &doc) {
    if (provided.length()) {
        doc["ts"] = provided;
    } else {
        doc["ts"] = iso_timestamp_now();
    }
}

}  // namespace

String make_uuid() {
    uint8_t raw[16];
    for (size_t i = 0; i < sizeof(raw); ++i) {
        raw[i] = static_cast<uint8_t>(esp_random() & 0xFF);
    }
    raw[6] = (raw[6] & 0x0F) | 0x40;
    raw[8] = (raw[8] & 0x3F) | 0x80;
    char buf[37];
    snprintf(buf, sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7], raw[8], raw[9], raw[10], raw[11], raw[12],
             raw[13], raw[14], raw[15]);
    return String(buf);
}

uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

String iso_timestamp_now() {
    time_t now = time(nullptr);
    struct tm tm;
    gmtime_r(&now, &tm);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return String(buf);
}

size_t encode_telemetry_env(const TelemetryEnv &telemetry, uint8_t *out, size_t capacity) {
    StaticJsonDocument<512> doc;
    doc["site"] = telemetry.site;
    doc["device"] = telemetry.device;
    doc["asset_id"] = telemetry.asset_id;
    ensure_timestamp(telemetry.timestamp_iso, doc);
    doc["fw"] = config::fw_version();
    doc["rssi_dbm"] = telemetry.rssi_dbm;
    doc["idempotency_key"] = telemetry.idempotency_key.length() ? telemetry.idempotency_key : make_uuid();
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["t_c"] = telemetry.temp_c;
    metrics["rh"] = telemetry.rh;
    metrics["mq135_ppm"] = telemetry.mq135_ppm;
    metrics["lux"] = telemetry.lux;
    if (telemetry.alarm) {
        doc["alarm"] = true;
    }
    return serialize(doc, out, capacity);
}

size_t encode_telemetry_power(const TelemetryPower &telemetry, uint8_t *out, size_t capacity) {
    StaticJsonDocument<512> doc;
    doc["site"] = telemetry.site;
    doc["device"] = telemetry.device;
    doc["asset_id"] = telemetry.asset_id;
    ensure_timestamp(telemetry.timestamp_iso, doc);
    doc["fw"] = config::fw_version();
    doc["rssi_dbm"] = telemetry.rssi_dbm;
    doc["idempotency_key"] = telemetry.idempotency_key.length() ? telemetry.idempotency_key : make_uuid();
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["voltage_v"] = telemetry.voltage;
    metrics["current_a"] = telemetry.current;
    metrics["power_w"] = telemetry.power_w;
    metrics["energy_Wh"] = telemetry.energy_Wh;
    if (!isnan(telemetry.pf)) {
        metrics["pf"] = telemetry.pf;
    }
    if (!isnan(telemetry.frequency)) {
        metrics["freq_hz"] = telemetry.frequency;
    }
    return serialize(doc, out, capacity);
}

size_t encode_telemetry_water(const TelemetryWater &telemetry, uint8_t *out, size_t capacity) {
    StaticJsonDocument<384> doc;
    doc["site"] = telemetry.site;
    doc["device"] = telemetry.device;
    doc["asset_id"] = telemetry.asset_id;
    ensure_timestamp(telemetry.timestamp_iso, doc);
    doc["fw"] = config::fw_version();
    doc["idempotency_key"] = telemetry.idempotency_key.length() ? telemetry.idempotency_key : make_uuid();
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["level_l"] = telemetry.level;
    metrics["flow_l_min"] = telemetry.flow_l_min;
    metrics["pump_on"] = telemetry.pump_on;
    return serialize(doc, out, capacity);
}

size_t encode_ack(const AckPayload &ack, uint8_t *out, size_t capacity) {
    StaticJsonDocument<256> doc;
    doc["asset_id"] = ack.asset_id;
    doc["correlation_id"] = ack.correlation_id;
    doc["ok"] = ack.ok;
    doc["message"] = ack.message;
    ensure_timestamp(ack.timestamp_iso, doc);
    return serialize(doc, out, capacity);
}

bool decode_command(const uint8_t *payload, size_t length, CommandMessage &out) {
    if (!payload || length == 0) {
        return false;
    }
    StaticJsonDocument<1024> doc;
    if (!deserialize(payload, length, doc)) {
        return false;
    }
    out.asset_id = doc["asset_id"].as<String>();
    out.correlation_id = doc["correlation_id"].as<String>();
    JsonObject relay = doc["relay"].as<JsonObject>();
    if (!relay.isNull()) {
        if (relay.containsKey("lamp")) {
            out.relay.has_lamp = true;
            out.relay.lamp_on = String(relay["lamp"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("fan")) {
            out.relay.has_fan = true;
            out.relay.fan_on = String(relay["fan"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("heater")) {
            out.relay.has_heater = true;
            out.relay.heater_on = String(relay["heater"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("door")) {
            out.relay.has_door = true;
            out.relay.door_open = String(relay["door"].as<const char*>()).equalsIgnoreCase("OPEN") ||
                                   String(relay["door"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("aux1")) {
            out.relay.has_aux1 = true;
            out.relay.aux1_on = String(relay["aux1"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("aux2")) {
            out.relay.has_aux2 = true;
            out.relay.aux2_on = String(relay["aux2"].as<const char*>()).equalsIgnoreCase("ON");
        }
        if (relay.containsKey("pump")) {
            out.pump_on_request = String(relay["pump"].as<const char*>()).equalsIgnoreCase("ON");
        }
    }
    JsonObject setpoints = doc["setpoints"].as<JsonObject>();
    if (!setpoints.isNull()) {
        if (setpoints.containsKey("t_c")) {
            out.setpoints.t_c = setpoints["t_c"].as<float>();
        }
        if (setpoints.containsKey("rh")) {
            out.setpoints.rh = setpoints["rh"].as<float>();
        }
        if (setpoints.containsKey("lux")) {
            out.setpoints.lux = setpoints["lux"].as<float>();
        }
    }
    JsonArray seq = doc["sequence"].as<JsonArray>();
    out.sequence_len = 0;
    if (!seq.isNull()) {
        for (JsonObject step : seq) {
            if (out.sequence_len >= (sizeof(out.sequence) / sizeof(out.sequence[0]))) {
                break;
            }
            SequenceStep &dst = out.sequence[out.sequence_len++];
            dst.action = step["act"].as<String>();
            if (step.containsKey("dur_s")) {
                dst.duration_s = step["dur_s"].as<uint32_t>();
            }
            if (step.containsKey("wait_s")) {
                dst.wait_s = step["wait_s"].as<uint32_t>();
            }
        }
    }
    if (doc.containsKey("reset_energy")) {
        out.reset_energy = doc["reset_energy"].as<bool>();
    }
    return true;
}

}  // namespace payload

