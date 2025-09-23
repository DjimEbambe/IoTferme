#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_idf_version.h>
#include <ArduinoJson.h>

constexpr const char *kBoardName = "device_sensor_simple_c6";
constexpr const char *kSiteId = "KIN-GOLIATH";
constexpr const char *kDeviceId = "c6-sensor-01";
constexpr const char *kAssetId = "A-PP-06";

const uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

unsigned long g_lastSend = 0;
uint32_t g_period_ms = 5000;
float g_phase = 0.0f;

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

void sendTelemetry() {
  StaticJsonDocument<256> doc;
  doc["site"] = kSiteId;
  doc["device"] = kDeviceId;
  doc["asset_id"] = kAssetId;
  doc["ts"] = isoTimestamp();
  JsonObject metrics = doc.createNestedObject("metrics");
  metrics["t_c"] = 26.0f + 1.5f * sinf(g_phase * 0.7f);
  metrics["rh"] = 62.0f + 6.0f * sinf(g_phase * 0.9f + 0.4f);
  metrics["lux"] = 150.0f + 50.0f * sinf(g_phase * 0.6f + 1.2f);
  metrics["mq135_ppm"] = 120.0f + 35.0f * sinf(g_phase * 1.4f);
  doc["idempotency_key"] = String(millis());
  String out;
  serializeJson(doc, out);
  esp_now_send(kBroadcastMac, reinterpret_cast<const uint8_t*>(out.c_str()), out.length());
  Serial.println(out);
  g_phase += 0.12f;
}

void sendAck(const String &correlation_id) {
  StaticJsonDocument<256> doc;
  doc["asset_id"] = kAssetId;
  doc["correlation_id"] = correlation_id;
  doc["ok"] = true;
  doc["message"] = "synthetic";
  doc["ts"] = isoTimestamp();
  String out;
  serializeJson(doc, out);
  esp_now_send(kBroadcastMac, reinterpret_cast<const uint8_t*>(out.c_str()), out.length());
}

void handleCommand(const JsonDocument &doc) {
  if (doc.containsKey("correlation_id")) {
    sendAck(doc["correlation_id"].as<String>());
  }
  if (doc.containsKey("period_ms")) {
    g_period_ms = doc["period_ms"].as<uint32_t>();
    if (g_period_ms < 1000) g_period_ms = 1000;
  }
}

void handleEspNowFrame(const uint8_t *mac, const uint8_t *data, int len) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
    handleCommand(doc);
  }
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
      }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) {
    delay(10);
  }
  Serial.printf("{\"event\":\"boot\",\"board\":\"%s\"}\n", kBoardName);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("{\"error\":\"esp_now_init\"}");
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
}

void loop() {
  processSerial();
  unsigned long now = millis();
  if (now - g_lastSend >= g_period_ms) {
    sendTelemetry();
    g_lastSend = now;
  }
  delay(50);
}
