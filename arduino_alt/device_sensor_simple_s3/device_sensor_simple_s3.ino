#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_idf_version.h>
#include <ArduinoJson.h>

#include "../common/config.h"
#include "../common/opts.h"

#if ENABLE_REAL_SENSORS
#include <Wire.h>
#include "../common/sensors_sht41.h"
#include "../common/sensor_light.h"
#include "../common/sensor_mq135.h"
#endif

#if ENABLE_REAL_SENSORS || ENABLE_RELAY_CONTROL
#include "../common/pins_trelay_s3.h"
#endif

#if ENABLE_RELAY_CONTROL
#include "../common/relays.h"
#endif

namespace {

constexpr const char *kBoardName = "device_sensor_simple_s3";
const config::DeviceIdentity kIdentity = config::kSensorIdentity;
const uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

unsigned long g_lastSend = 0;
uint32_t g_period_ms = 5000;
float g_phase = 0.0f;

bool g_realSensorsEnabled = ENABLE_REAL_SENSORS != 0;
bool g_relayControlEnabled = ENABLE_RELAY_CONTROL != 0;

#if ENABLE_REAL_SENSORS
sensors::Sht41Wrapper g_sht;
sensors::LightSensor g_light;
sensors::Mq135Sensor g_mq135;
bool g_shtReady = false;
bool g_mq135Ready = false;
#endif

#if ENABLE_RELAY_CONTROL
relays::RelayBank g_relays;
#endif

struct TelemetrySample {
    float temp_c = NAN;
    float rh = NAN;
    float lux = NAN;
    float mq135_ppm = NAN;
    bool used_real = false;
};

String isoTimestamp() {
    unsigned long ms = millis();
    unsigned long sec = ms / 1000UL;
    unsigned long rem_ms = ms % 1000UL;
    unsigned long minutes = sec / 60UL;
    unsigned long hours = minutes / 60UL;
    unsigned long days = hours / 24UL;
    hours %= 24UL;
    minutes %= 60UL;
    sec %= 60UL;
    char buf[32];
    snprintf(buf, sizeof(buf), "2025-01-%02luT%02lu:%02lu:%02lu.%03luZ", (days % 28) + 1, hours, minutes, sec, rem_ms);
    return String(buf);
}

void broadcastDocument(const JsonDocument &doc) {
    String out;
    serializeJson(doc, out);
    esp_now_send(kBroadcastMac, reinterpret_cast<const uint8_t *>(out.c_str()), out.length());
    Serial.println(out);
}

#if ENABLE_REAL_SENSORS
void ensureSensorsReady() {
    if (!g_realSensorsEnabled) {
        return;
    }
    if (!g_shtReady) {
        g_shtReady = g_sht.begin();
        if (!g_shtReady) {
            Serial.println(F("{\"event\":\"error\",\"detail\":\"sht41_init\"}"));
        }
    }
    if (!g_mq135Ready) {
        g_mq135Ready = g_mq135.begin(pins_trelay_s3::MQ135_ADC, "r0");
        if (!g_mq135Ready) {
            Serial.println(F("{\"event\":\"error\",\"detail\":\"mq135_init\"}"));
        }
    }
}
#endif

TelemetrySample collectTelemetry() {
    TelemetrySample sample;
#if ENABLE_REAL_SENSORS
    if (g_realSensorsEnabled) {
        ensureSensorsReady();
        if (g_shtReady) {
            float temp = NAN;
            float rh = NAN;
            if (g_sht.read(temp, rh)) {
                sample.temp_c = temp;
                sample.rh = rh;
                sample.used_real = true;
            }
        }
        sample.lux = g_light.read_lux(config::kAdcOversample);
        if (g_mq135Ready && g_mq135.is_warmup_done()) {
            sample.mq135_ppm = g_mq135.read_ppm(sample.temp_c, sample.rh);
        }
    }
#endif
    if (!sample.used_real) {
        sample.temp_c = 27.0f + 2.0f * sinf(g_phase);
        sample.rh = 60.0f + 5.0f * sinf(g_phase * 0.8f + 0.3f);
        sample.lux = 180.0f + 40.0f * sinf(g_phase * 0.5f + 1.0f);
        sample.mq135_ppm = 140.0f + 30.0f * sinf(g_phase * 1.3f);
    } else {
        if (isnan(sample.lux)) {
            sample.lux = 0.0f;
        }
    }
    return sample;
}

void sendTelemetry() {
    TelemetrySample sample = collectTelemetry();

    StaticJsonDocument<512> doc;
    doc["type"] = "telemetry";
    doc["site"] = config::kSiteId;
    doc["device"] = kIdentity.device_id;
    doc["asset_id"] = kIdentity.asset_id;
    doc["ts"] = isoTimestamp();
    doc["fw"] = config::fw_version();
    doc["idempotency_key"] = String(millis());
    doc["real_sensors"] = sample.used_real;

    JsonObject metrics = doc.createNestedObject("metrics");
    if (!isnan(sample.temp_c)) {
        metrics["t_c"] = sample.temp_c;
    }
    if (!isnan(sample.rh)) {
        metrics["rh"] = sample.rh;
    }
    if (!isnan(sample.lux)) {
        metrics["lux"] = sample.lux;
    }
    if (!isnan(sample.mq135_ppm)) {
        metrics["mq135_ppm"] = sample.mq135_ppm;
    }

    broadcastDocument(doc);
    g_phase += 0.15f;
}

void sendEvent(const char *event, const char *detail = nullptr) {
    StaticJsonDocument<256> doc;
    doc["type"] = "event";
    doc["event"] = event;
    doc["device"] = kIdentity.device_id;
    doc["asset_id"] = kIdentity.asset_id;
    doc["ts"] = isoTimestamp();
    if (detail) {
        doc["detail"] = detail;
    }
    doc["real_sensors"] = g_realSensorsEnabled;
    doc["relay_control"] = g_relayControlEnabled;
    broadcastDocument(doc);
}

void sendFeatureSnapshot(const char *reason) {
    StaticJsonDocument<256> doc;
    doc["type"] = "event";
    doc["event"] = "features";
    doc["device"] = kIdentity.device_id;
    doc["asset_id"] = kIdentity.asset_id;
    doc["ts"] = isoTimestamp();
    doc["real_sensors"] = g_realSensorsEnabled;
    doc["relay_control"] = g_relayControlEnabled;
    if (reason) {
        doc["reason"] = reason;
    }
    broadcastDocument(doc);
}

#if ENABLE_RELAY_CONTROL
void sendRelayStatus(const char *reason) {
    StaticJsonDocument<256> doc;
    doc["type"] = "event";
    doc["event"] = "relay_status";
    doc["device"] = kIdentity.device_id;
    doc["asset_id"] = kIdentity.asset_id;
    doc["ts"] = isoTimestamp();
    if (reason) {
        doc["reason"] = reason;
    }
    JsonObject states = doc.createNestedObject("states");
    states["lamp"] = g_relays.get(relays::kLamp);
    states["fan"] = g_relays.get(relays::kFan);
    states["heater"] = g_relays.get(relays::kHeater);
    states["door"] = g_relays.get(relays::kDoor);
    states["aux1"] = g_relays.get(relays::kAux1);
    states["aux2"] = g_relays.get(relays::kAux2);
    broadcastDocument(doc);
}
#endif

void sendAck(const String &correlation_id, bool ok, const char *message) {
    StaticJsonDocument<256> doc;
    doc["type"] = "ack";
    doc["asset_id"] = kIdentity.asset_id;
    doc["device"] = kIdentity.device_id;
    doc["correlation_id"] = correlation_id;
    doc["ok"] = ok;
    if (message) {
        doc["message"] = message;
    }
    doc["ts"] = isoTimestamp();
    broadcastDocument(doc);
}

bool parseRelayState(JsonVariantConst value, bool &state) {
    if (value.is<bool>()) {
        state = value.as<bool>();
        return true;
    }
    if (value.is<int>()) {
        state = value.as<int>() != 0;
        return true;
    }
    if (value.is<const char*>()) {
        String text = value.as<const char*>();
        text.toUpperCase();
        if (text == "ON" || text == "OPEN" || text == "1" || text == "TRUE") {
            state = true;
            return true;
        }
        if (text == "OFF" || text == "CLOSE" || text == "CLOSED" || text == "0" || text == "FALSE") {
            state = false;
            return true;
        }
    }
    return false;
}

void handleFeatureToggle(const JsonDocument &doc, const String &correlation_id) {
    bool ok = true;
    const char *message = "features_updated";
    if (doc.containsKey("real_sensors")) {
        bool request = doc["real_sensors"].as<bool>();
#if ENABLE_REAL_SENSORS
        g_realSensorsEnabled = request;
#else
        if (request) {
            ok = false;
            message = "real_sensors_unavailable";
        }
#endif
    }
    if (doc.containsKey("relay_control")) {
        bool request = doc["relay_control"].as<bool>();
#if ENABLE_RELAY_CONTROL
        g_relayControlEnabled = request;
        if (!g_relayControlEnabled) {
#if ENABLE_RELAY_CONTROL
            g_relays.force_all_off();
#endif
        }
#else
        if (request) {
            ok = false;
            message = ok ? "relay_control_unavailable" : message;
        }
#endif
    }
    sendFeatureSnapshot("remote");
    if (!correlation_id.isEmpty()) {
        sendAck(correlation_id, ok, message);
    }
}

void handleCommand(const JsonDocument &doc) {
    const char *type = doc["type"].as<const char*>();
    String correlation = doc["correlation_id"].as<String>();

    if (type && strcmp(type, "feature") == 0) {
        handleFeatureToggle(doc, correlation);
        return;
    }

    bool ok = true;
    const char *message = "applied";

    if (doc.containsKey("period_ms")) {
        uint32_t period = doc["period_ms"].as<uint32_t>();
        if (period >= 1000) {
            g_period_ms = period;
        } else {
            ok = false;
            message = "period_invalid";
        }
    }

    if (doc.containsKey("relay")) {
        JsonObjectConst relay = doc["relay"].as<JsonObjectConst>();
#if ENABLE_RELAY_CONTROL
        if (g_relayControlEnabled) {
            bool local_ok = true;
            if (relay.containsKey("lamp")) {
                bool state = false;
                local_ok &= parseRelayState(relay["lamp"], state) && g_relays.command(relays::kLamp, state);
            }
            if (relay.containsKey("fan")) {
                bool state = false;
                local_ok &= parseRelayState(relay["fan"], state) && g_relays.command(relays::kFan, state);
            }
            if (relay.containsKey("heater")) {
                bool state = false;
                local_ok &= parseRelayState(relay["heater"], state) && g_relays.command(relays::kHeater, state);
            }
            if (relay.containsKey("door")) {
                bool state = false;
                local_ok &= parseRelayState(relay["door"], state) && g_relays.command(relays::kDoor, state);
            }
            if (relay.containsKey("aux1")) {
                bool state = false;
                local_ok &= parseRelayState(relay["aux1"], state) && g_relays.command(relays::kAux1, state);
            }
            if (relay.containsKey("aux2")) {
                bool state = false;
                local_ok &= parseRelayState(relay["aux2"], state) && g_relays.command(relays::kAux2, state);
            }
            if (!local_ok) {
                ok = false;
                message = "relay_failed";
            }
            sendRelayStatus("command");
        } else {
            ok = false;
            message = "relay_disabled";
        }
#else
        (void)relay;
        ok = false;
        message = "relay_unavailable";
#endif
    }

    if (!correlation.isEmpty()) {
        sendAck(correlation, ok, message);
    }
}

void handleEspNowFrame(const uint8_t *mac, const uint8_t *data, int len) {
    (void)mac;
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        sendEvent("error", "json_parse");
        return;
    }
    handleCommand(doc);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    const uint8_t *src = info ? info->src_addr : nullptr;
    handleEspNowFrame(src, data, len);
}
#else
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
    handleEspNowFrame(mac, data, len);
}
#endif

void processSerial() {
    static String line;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            line.trim();
            if (line.equalsIgnoreCase("send")) {
                sendTelemetry();
            } else if (line.startsWith("period")) {
                uint32_t value = line.substring(6).toInt();
                if (value >= 1000) {
                    g_period_ms = value;
                }
            } else if (line.equalsIgnoreCase("real on")) {
#if ENABLE_REAL_SENSORS
                g_realSensorsEnabled = true;
                sendFeatureSnapshot("serial");
#endif
            } else if (line.equalsIgnoreCase("real off")) {
                g_realSensorsEnabled = false;
                sendFeatureSnapshot("serial");
            } else if (line.equalsIgnoreCase("relay on")) {
#if ENABLE_RELAY_CONTROL
                g_relayControlEnabled = true;
                sendFeatureSnapshot("serial");
#endif
            } else if (line.equalsIgnoreCase("relay off")) {
                g_relayControlEnabled = false;
#if ENABLE_RELAY_CONTROL
                g_relays.force_all_off();
#endif
                sendFeatureSnapshot("serial");
            }
            line = "";
        } else if (c != '\r') {
            line += c;
        }
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) {
        delay(10);
    }
    Serial.printf("{\"event\":\"boot\",\"board\":\"%s\"}\n", kBoardName);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

#if ENABLE_REAL_SENSORS
    Wire.begin(pins_trelay_s3::I2C_SDA, pins_trelay_s3::I2C_SCL, 100000);
    g_light.begin(pins_trelay_s3::LIGHT_SENSOR_ADC);
#endif

#if ENABLE_RELAY_CONTROL
    g_relays.begin(pins_trelay_s3::RELAY_PINS, sizeof(pins_trelay_s3::RELAY_PINS));
#endif

    if (esp_now_init() != ESP_OK) {
        Serial.println(F("{\"error\":\"esp_now_init\"}"));
        while (true) {
            delay(1000);
        }
    }
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, kBroadcastMac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
    esp_now_register_recv_cb(onEspNowRecv);

    sendEvent("boot", kBoardName);

    sendFeatureSnapshot("boot");
#if ENABLE_RELAY_CONTROL
    sendRelayStatus("boot");
#endif
}

void loop() {
    processSerial();
    unsigned long now = millis();
    if (now - g_lastSend >= g_period_ms) {
        sendTelemetry();
        g_lastSend = now;
    }
#if ENABLE_RELAY_CONTROL
    g_relays.loop();
#endif
    delay(50);
}
