#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_idf_version.h>
#include <ArduinoJson.h>

struct PeerEntry {
  uint8_t mac[6];
  String asset;
};

constexpr const char *kBoardName = "gateway_simple_s3";
constexpr uint8_t kMaxPeers = 8;
PeerEntry g_peers[kMaxPeers];
uint8_t g_peer_count = 0;

String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

bool parseMac(const String &text, uint8_t *out) {
  unsigned int values[6];
  if (sscanf(text.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    out[i] = static_cast<uint8_t>(values[i]);
  }
  return true;
}

void addBroadcastPeer() {
  uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
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

void replyJson(const JsonDocument &doc) {
  String out;
  serializeJson(doc, out);
  Serial.println(out);
}

void sendEvent(const char *type, const char *detail = nullptr) {
  StaticJsonDocument<256> doc;
  doc["event"] = type;
  doc["board"] = kBoardName;
  if (detail) {
    doc["detail"] = detail;
  }
  replyJson(doc);
}

void handleAddPeer(const JsonDocument &doc) {
  const char *mac_str = doc["mac"].as<const char*>();
  const char *asset = doc["asset"].as<const char*>();
  if (!mac_str || !asset) {
    sendEvent("error", "add_peer missing fields");
    return;
  }
  uint8_t mac[6];
  if (!parseMac(String(mac_str), mac)) {
    sendEvent("error", "invalid mac");
    return;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_err_t err = esp_now_add_peer(&peer);
  if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
    sendEvent("error", "esp_now_add_peer failed");
    return;
  }
  PeerEntry *entry = findPeerByAsset(asset);
  if (!entry && g_peer_count < kMaxPeers) {
    entry = &g_peers[g_peer_count++];
  }
  if (entry) {
    memcpy(entry->mac, mac, 6);
    entry->asset = asset;
  }
  StaticJsonDocument<256> res;
  res["event"] = "peer_added";
  res["asset"] = asset;
  res["mac"] = mac_str;
  replyJson(res);
}

void handleSend(const JsonDocument &doc) {
  const char *asset = doc["asset"].as<const char*>();
  const char *mac_str = doc["mac"].as<const char*>();
  JsonVariantConst payload = doc["payload"];
  if (payload.isNull()) {
    sendEvent("error", "send missing payload");
    return;
  }
  uint8_t target_mac[6];
  PeerEntry *peer = nullptr;
  if (asset) {
    peer = findPeerByAsset(asset);
  }
  if (!peer && mac_str) {
    if (!parseMac(String(mac_str), target_mac)) {
      sendEvent("error", "invalid mac");
      return;
    }
  } else if (peer) {
    memcpy(target_mac, peer->mac, 6);
  } else {
    sendEvent("error", "unknown target");
    return;
  }
  String out;
  serializeJson(payload, out);
  esp_now_send(target_mac, reinterpret_cast<const uint8_t*>(out.c_str()), out.length());
  StaticJsonDocument<256> res;
  res["event"] = "sent";
  res["target"] = macToString(target_mac);
  replyJson(res);
}

void handleListPeers() {
  StaticJsonDocument<512> doc;
  doc["event"] = "peers";
  JsonArray arr = doc.createNestedArray("items");
  for (uint8_t i = 0; i < g_peer_count; ++i) {
    JsonObject item = arr.createNestedObject();
    item["asset"] = g_peers[i].asset;
    item["mac"] = macToString(g_peers[i].mac);
  }
  replyJson(doc);
}

void handleSerialLine(const String &line) {
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    sendEvent("error", "json_parse");
    return;
  }
  const char *cmd = doc["cmd"].as<const char*>();
  if (!cmd) {
    sendEvent("error", "missing cmd");
    return;
  }
  if (strcmp(cmd, "add_peer") == 0) {
    handleAddPeer(doc);
  } else if (strcmp(cmd, "send") == 0) {
    handleSend(doc);
  } else if (strcmp(cmd, "list") == 0) {
    handleListPeers();
  } else {
    sendEvent("error", "unknown cmd");
  }
}

void processSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      line.trim();
      if (line.length()) {
        handleSerialLine(line);
      }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
}

void handleEspNowFrame(const uint8_t *mac, const uint8_t *data, int len) {
  StaticJsonDocument<512> frame;
  frame["event"] = "espnow_rx";
  static const uint8_t dummy_mac[6] = {0};
  const uint8_t *src = mac ? mac : dummy_mac;
  frame["from_mac"] = macToString(src);
  PeerEntry *entry = findPeerByMac(mac);
  if (entry) {
    frame["asset"] = entry->asset;
  }
  StaticJsonDocument<512> payload;
  if (deserializeJson(payload, data, len) == DeserializationError::Ok) {
    frame["payload"] = payload;
  } else {
    frame["raw"] = String((const char*)data).substring(0, len);
  }
  replyJson(frame);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info ? info->src_addr : nullptr;
  handleEspNowFrame(mac, data, len);
}
#else
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  handleEspNowFrame(mac, data, len);
}
#endif

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) {
    delay(10);
  }
  sendEvent("boot");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    sendEvent("error", "esp_now_init");
    while (true) {
      delay(1000);
    }
  }
  addBroadcastPeer();
  esp_now_register_recv_cb(onEspNowRecv);
}

void loop() {
  processSerial();
  delay(10);
}
