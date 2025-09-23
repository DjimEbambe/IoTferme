#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_idf_version.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <Preferences.h>

struct PeerEntry {
  uint8_t mac[6];
  String asset;
};

constexpr const char *kBoardName = "gateway_simple_c6";
constexpr uint8_t kMaxPeers = 8;
constexpr uint32_t kSerialBaud = 921600;
constexpr const char *kPrefsNamespace = "gw_cfg";
constexpr const char *kPrefsKeyMac = "sta_mac";

PeerEntry g_peers[kMaxPeers];
uint8_t g_peer_count = 0;
uint8_t g_rx_buffer[512];
size_t g_rx_len = 0;
Preferences g_prefs;
bool g_restart_pending = false;
uint32_t g_restart_deadline = 0;

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

String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

bool parseMac(const String &text, uint8_t *out) {
  unsigned int values[6];
  if (sscanf(text.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    out[i] = static_cast<uint8_t>(values[i]);
  }
  return true;
}

PeerEntry *findPeerByAsset(const String &asset) {
  for (uint8_t i = 0; i < g_peer_count; ++i) {
    if (g_peers[i].asset.equalsIgnoreCase(asset)) {
      return &g_peers[i];
    }
  }
  return nullptr;
}

PeerEntry *findPeerByMac(const uint8_t *mac) {
  for (uint8_t i = 0; i < g_peer_count; ++i) {
    if (memcmp(g_peers[i].mac, mac, 6) == 0) {
      return &g_peers[i];
    }
  }
  return nullptr;
}

PeerEntry *ensurePeerSlot(const char *asset) {
  if (!asset) {
    return nullptr;
  }
  PeerEntry *entry = findPeerByAsset(asset);
  if (entry) {
    return entry;
  }
  if (g_peer_count >= kMaxPeers) {
    return nullptr;
  }
  entry = &g_peers[g_peer_count++];
  entry->asset = asset;
  return entry;
}

uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
      crc &= 0xFFFF;
    }
  }
  return crc;
}

size_t cobsEncode(const uint8_t *input, size_t length, uint8_t *output, size_t capacity) {
  size_t read_index = 0;
  size_t write_index = 1;
  size_t code_index = 0;
  uint8_t code = 1;
  if (!output || capacity == 0) {
    return 0;
  }
  while (read_index < length && write_index < capacity) {
    if (input[read_index] == 0) {
      output[code_index] = code;
      code = 1;
      code_index = write_index++;
      read_index++;
      if (write_index >= capacity) {
        return 0;
      }
    } else {
      output[write_index++] = input[read_index++];
      code++;
      if (code == 0xFF) {
        output[code_index] = code;
        code = 1;
        code_index = write_index++;
        if (write_index >= capacity) {
          return 0;
        }
      }
    }
  }
  output[code_index] = code;
  return write_index;
}

size_t cobsDecode(const uint8_t *input, size_t length, uint8_t *output, size_t capacity) {
  size_t read_index = 0;
  size_t write_index = 0;
  while (read_index < length) {
    uint8_t code = input[read_index++];
    if (code == 0 || read_index + code - 1 > length) {
      return 0;
    }
    for (uint8_t i = 1; i < code; ++i) {
      if (write_index >= capacity) {
        return 0;
      }
      output[write_index++] = input[read_index++];
    }
    if (code < 0xFF && read_index < length) {
      if (write_index >= capacity) {
        return 0;
      }
      output[write_index++] = 0;
    }
  }
  return write_index;
}

void sendFrame(const JsonDocument &doc) {
  uint8_t payload[512];
  size_t len = serializeMsgPack(doc, payload, sizeof(payload));
  if (len == 0 || len + 2 > sizeof(payload)) {
    return;
  }
  uint16_t crc = crc16_ccitt(payload, len);
  payload[len++] = static_cast<uint8_t>(crc >> 8);
  payload[len++] = static_cast<uint8_t>(crc & 0xFF);
  uint8_t frame[600];
  size_t encoded = cobsEncode(payload, len, frame, sizeof(frame) - 1);
  if (encoded == 0 || encoded >= sizeof(frame)) {
    return;
  }
  frame[encoded++] = 0;
  Serial.write(frame, encoded);
}

void sendEvent(const char *type, const char *detail = nullptr) {
  StaticJsonDocument<256> doc;
  doc["type"] = "event";
  doc["event"] = type;
  doc["board"] = kBoardName;
  if (detail) {
    doc["detail"] = detail;
  }
  doc["ts"] = isoTimestamp();
  sendFrame(doc);
}

void sendStatus(const char *status) {
  StaticJsonDocument<256> doc;
  doc["type"] = "status";
  doc["status"] = status;
  doc["board"] = kBoardName;
  doc["ts"] = isoTimestamp();
  sendFrame(doc);
}

void registerPeer(const uint8_t *mac, const char *asset) {
  if (!mac || !asset) {
    return;
  }
  PeerEntry *entry = findPeerByMac(mac);
  if (!entry) {
    entry = ensurePeerSlot(asset);
  }
  if (!entry) {
    return;
  }
  memcpy(entry->mac, mac, 6);
  entry->asset = asset;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void handleListPeers() {
  StaticJsonDocument<512> doc;
  doc["type"] = "event";
  doc["event"] = "peers";
  JsonArray arr = doc.createNestedArray("items");
  for (uint8_t i = 0; i < g_peer_count; ++i) {
    JsonObject peer = arr.createNestedObject();
    peer["asset"] = g_peers[i].asset;
    peer["mac"] = macToString(g_peers[i].mac);
  }
  doc["ts"] = isoTimestamp();
  sendFrame(doc);
}

void handleAddPeer(const JsonDocument &doc) {
  const char *mac_str = doc["mac"].as<const char*>();
  const char *asset = doc["asset_id"].as<const char*>();
  if (!asset) {
    asset = doc["asset"].as<const char*>();
  }
  if (!mac_str || !asset) {
    sendEvent("error", "add_peer_missing");
    return;
  }
  uint8_t mac[6];
  if (!parseMac(String(mac_str), mac)) {
    sendEvent("error", "add_peer_invalid_mac");
    return;
  }
  registerPeer(mac, asset);
  StaticJsonDocument<256> res;
  res["type"] = "event";
  res["event"] = "peer_added";
  res["asset"] = asset;
  res["mac"] = mac_str;
  res["ts"] = isoTimestamp();
  sendFrame(res);
}

void forwardToDevice(const JsonDocument &doc, const uint8_t *mac) {
  String payload;
  serializeJson(doc, payload);
  esp_now_send(mac, reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());
}

void handleCommand(const JsonDocument &doc) {
  const char *asset = doc["asset_id"].as<const char*>();
  if (!asset) {
    asset = doc["asset"].as<const char*>();
  }
  const char *corr = doc["correlation_id"].as<const char*>();
  const char *mac_override = doc["mac"].as<const char*>();
  if (!asset && !mac_override) {
    sendEvent("error", "cmd_missing_target");
    return;
  }
  uint8_t target_mac[6];
  PeerEntry *entry = asset ? findPeerByAsset(asset) : nullptr;
  bool have_mac = false;
  if (entry) {
    memcpy(target_mac, entry->mac, 6);
    have_mac = true;
  }
  if (!have_mac && mac_override) {
    have_mac = parseMac(String(mac_override), target_mac);
  }
  if (!have_mac) {
    if (asset && corr) {
      StaticJsonDocument<256> ack;
      ack["type"] = "ack";
      ack["asset_id"] = asset;
      ack["correlation_id"] = corr;
      ack["ok"] = false;
      ack["message"] = "asset_not_found";
      ack["ts"] = isoTimestamp();
      sendFrame(ack);
    }
    sendEvent("error", "cmd_no_peer");
    return;
  }
  if (asset && !entry) {
    registerPeer(target_mac, asset);
  }
  StaticJsonDocument<768> outbound;
  outbound.set(doc);
  outbound.remove("type");
  outbound.remove("mac");
  forwardToDevice(outbound, target_mac);
}

void broadcastTimeSync(const JsonDocument &doc) {
  StaticJsonDocument<256> outbound;
  outbound.set(doc);
  outbound.remove("type");
  uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  forwardToDevice(outbound, broadcast);
  for (uint8_t i = 0; i < g_peer_count; ++i) {
    forwardToDevice(outbound, g_peers[i].mac);
  }
}

bool loadMacFromPrefs(uint8_t *out) {
  String stored = g_prefs.getString(kPrefsKeyMac, "");
  if (stored.length() != 17) {
    return false;
  }
  if (!parseMac(stored, out)) {
    return false;
  }
  return true;
}

void handleSetMac(const JsonDocument &doc) {
  const char *mac_str = doc["mac"].as<const char*>();
  if (!mac_str) {
    sendEvent("error", "set_mac_missing");
    return;
  }
  uint8_t mac[6];
  if (!parseMac(String(mac_str), mac)) {
    sendEvent("error", "set_mac_invalid");
    return;
  }
  bool persist = doc["persist"].as<bool>();
  if (persist) {
    g_prefs.putString(kPrefsKeyMac, mac_str);
  } else {
    g_prefs.remove(kPrefsKeyMac);
  }
  StaticJsonDocument<256> res;
  res["type"] = "event";
  res["event"] = "set_mac";
  res["mac"] = mac_str;
  res["persist"] = persist;
  res["reboot"] = true;
  res["ts"] = isoTimestamp();
  sendFrame(res);
  Serial.flush();
  delay(50);
  g_restart_pending = true;
  g_restart_deadline = millis() + 200;
}

void handleHostFrame(const uint8_t *data, size_t len) {
  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeMsgPack(doc, data, len);
  if (err) {
    sendEvent("error", "host_decode");
    return;
  }
  const char *type = doc["type"].as<const char*>();
  if (!type) {
    sendEvent("error", "host_missing_type");
    return;
  }
  if (strcmp(type, "cmd") == 0) {
    handleCommand(doc);
  } else if (strcmp(type, "add_peer") == 0) {
    handleAddPeer(doc);
  } else if (strcmp(type, "list_peers") == 0) {
    handleListPeers();
  } else if (strcmp(type, "time_sync") == 0) {
    broadcastTimeSync(doc);
  } else if (strcmp(type, "ping") == 0) {
    const char *asset = doc["asset_id"].as<const char*>();
    PeerEntry *entry = asset ? findPeerByAsset(asset) : nullptr;
    if (entry) {
      StaticJsonDocument<128> ping;
      ping["ping"] = doc["ts"].as<const char*>();
      forwardToDevice(ping, entry->mac);
    }
  } else if (strcmp(type, "cfg") == 0) {
    const char *op = doc["op"].as<const char*>();
    if (op && strcmp(op, "set_mac") == 0) {
      handleSetMac(doc);
    } else {
      sendEvent("error", "cfg_unknown");
    }
  } else {
    sendEvent("error", "host_unknown_type");
  }
}

void processSerial() {
  while (Serial.available()) {
    uint8_t byte = static_cast<uint8_t>(Serial.read());
    if (byte == 0x00) {
      if (g_rx_len == 0) {
        continue;
      }
      uint8_t decoded[512];
      size_t decoded_len = cobsDecode(g_rx_buffer, g_rx_len, decoded, sizeof(decoded));
      g_rx_len = 0;
      if (decoded_len < 2) {
        continue;
      }
      uint16_t expected = static_cast<uint16_t>(decoded[decoded_len - 2] << 8) | decoded[decoded_len - 1];
      uint16_t actual = crc16_ccitt(decoded, decoded_len - 2);
      if (expected != actual) {
        sendEvent("crc_error");
        continue;
      }
      handleHostFrame(decoded, decoded_len - 2);
    } else if (g_rx_len < sizeof(g_rx_buffer)) {
      g_rx_buffer[g_rx_len++] = byte;
    } else {
      g_rx_len = 0;
      sendEvent("rx_overflow");
    }
  }
}

void forwardTelemetryToHost(JsonDocument &doc, const uint8_t *mac, int8_t rssi) {
  doc["mac"] = macToString(mac);
  doc["rssi_dbm"] = rssi;
  if (!doc.containsKey("type")) {
    if (doc.containsKey("correlation_id")) {
      doc["type"] = "ack";
    } else {
      doc["type"] = "telemetry";
    }
  }
  if (!doc.containsKey("ts")) {
    doc["ts"] = isoTimestamp();
  }
  const char *asset = doc["asset_id"].as<const char*>();
  if (asset) {
    registerPeer(mac, asset);
  } else {
    PeerEntry *entry = findPeerByMac(mac);
    if (entry) {
      doc["asset_id"] = entry->asset;
    }
  }
  sendFrame(doc);
}

void handleEspNowFrame(const uint8_t *mac, const uint8_t *data, int len, int8_t rssi) {
  StaticJsonDocument<768> payload;
  if (deserializeJson(payload, data, len) != DeserializationError::Ok) {
    sendEvent("parse_error");
    return;
  }
  forwardTelemetryToHost(payload, mac, rssi);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *src = info ? info->src_addr : nullptr;
  int8_t rssi = info && info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  if (src) {
    handleEspNowFrame(src, data, len, rssi);
  }
}
#else
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  handleEspNowFrame(mac, data, len, 0);
}
#endif

void applyConfiguredMac() {
  uint8_t mac[6];
  if (loadMacFromPrefs(mac)) {
    esp_wifi_set_mac(WIFI_IF_STA, mac);
    StaticJsonDocument<192> doc;
    doc["type"] = "event";
    doc["event"] = "mac_loaded";
    doc["mac"] = macToString(mac);
    doc["ts"] = isoTimestamp();
    sendFrame(doc);
  }
}

void setup() {
  Serial.begin(kSerialBaud);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) {
    delay(10);
  }
  g_prefs.begin(kPrefsNamespace, false);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  applyConfiguredMac();

  if (esp_now_init() != ESP_OK) {
    sendEvent("error", "esp_now_init");
    while (true) {
      delay(1000);
    }
  }
  uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onEspNowRecv);
  sendStatus("ready");
}

void loop() {
  processSerial();
  if (g_restart_pending && millis() > g_restart_deadline) {
    esp_restart();
  }
  delay(5);
}
