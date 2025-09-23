#include "espnow_link.h"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_idf_version.h>
#include <esp_wifi.h>

#include "debug_log.h"
#include "payload.h"

namespace espnow_link {

static const char *kPrefsNs = "gwroute";
static const char *kPrefsKey = "table";

EspNowLink *EspNowLink::instance_ = nullptr;

namespace {

void mac_to_str(const uint8_t *mac, char *out, size_t out_len) {
    snprintf(out, out_len, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

}  // namespace

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
void EspNowLink::recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (!instance_) {
        return;
    }
    int rssi = 0;
    const uint8_t *mac = nullptr;
    if (info) {
        mac = info->src_addr;
        if (info->rx_ctrl) {
            rssi = info->rx_ctrl->rssi;
        }
    }
    instance_->handle_recv(mac, data, len, rssi);
}
#else
void EspNowLink::recv_callback(const uint8_t *mac, const uint8_t *data, int len) {
    if (!instance_) {
        return;
    }
    instance_->handle_recv(mac, data, len, 0);
}
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void EspNowLink::send_callback(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    if (!instance_) {
        return;
    }
    const uint8_t *mac = nullptr;
    if (info) {
        mac = info->des_addr;
    }
    instance_->handle_send_status(mac, status);
}
#else
void EspNowLink::send_callback(const uint8_t *mac, esp_now_send_status_t status) {
    if (!instance_) {
        return;
    }
    instance_->handle_send_status(mac, status);
}
#endif

EspNowLink &instance() {
    static EspNowLink link;
    return link;
}

bool EspNowLink::begin(bool long_range) {
    if (instance_ != nullptr && instance_ != this) {
        return true;
    }
    instance_ = this;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        LOGE("ESP-NOW init failed");
        return false;
    }

    if (esp_now_set_pmk(config::kEspNowPrimaryKey) != ESP_OK) {
        LOGE("PMK set failed");
    }

    esp_now_register_send_cb(send_callback);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
    esp_now_register_recv_cb(recv_callback);
#else
    esp_now_register_recv_cb(recv_callback);
#endif

#ifdef ESP_NOW_WIFI_PWR_CTRL
    esp_wifi_set_max_tx_power(long_range ? 78 : 70);
#endif

    load_routing();

    return true;
}

void EspNowLink::set_receive_handler(ReceiveHandler handler) {
    receive_handler_ = handler;
}

void EspNowLink::set_ack_timeout_handler(AckTimeoutHandler handler) {
    ack_timeout_handler_ = handler;
}

void EspNowLink::set_peer_event_handler(PeerEventHandler handler) {
    peer_event_handler_ = handler;
}

bool EspNowLink::send(const uint8_t *mac, const uint8_t *payload, size_t length, const String &correlation_id) {
    if (!mac || !payload || length == 0 || length > config::kEspNowPayloadMax) {
        return false;
    }
    if (!esp_now_is_peer_exist(mac)) {
        LOGW("Peer missing for send");
        return false;
    }
    esp_err_t err = esp_now_send(mac, payload, length);
    if (err != ESP_OK) {
        LOGE("esp_now_send err=%d", err);
        return false;
    }
    if (correlation_id.length()) {
        AckTracker *slot = nullptr;
        for (auto &candidate : ack_slots_) {
            if (!candidate.in_use || candidate.correlation_id == correlation_id) {
                slot = &candidate;
                break;
            }
        }
        if (slot) {
            slot->in_use = true;
            slot->correlation_id = correlation_id;
            memcpy(slot->mac, mac, 6);
            slot->retries = 0;
            slot->last_tx_ms = millis();
            slot->length = length;
            memcpy(slot->payload, payload, length);
        } else {
            LOGW("ack table full");
        }
    }
    return true;
}

void EspNowLink::notify_ack_received(const String &correlation_id) {
    if (!correlation_id.length()) {
        return;
    }
    for (auto &slot : ack_slots_) {
        if (slot.in_use && slot.correlation_id == correlation_id) {
            slot.in_use = false;
            slot.correlation_id = "";
            memset(slot.mac, 0, sizeof(slot.mac));
            slot.retries = 0;
            slot.last_tx_ms = 0;
            slot.length = 0;
            break;
        }
    }
}

void EspNowLink::loop() {
    uint32_t now = millis();
    for (auto &slot : ack_slots_) {
        if (!slot.in_use) {
            continue;
        }
        if (now - slot.last_tx_ms >= CMD_ACK_TIMEOUT_MS) {
            if (slot.retries >= CMD_RETRY_LIMIT) {
                if (ack_timeout_handler_) {
                    ack_timeout_handler_(slot);
                }
                LOGW("Ack timeout exceeded %s", slot.correlation_id.c_str());
                slot.in_use = false;
                continue;
            }
            slot.retries++;
            slot.last_tx_ms = now;
            esp_now_send(slot.mac, slot.payload, slot.length);
            if (ack_timeout_handler_) {
                ack_timeout_handler_(slot);
            }
        }
    }

    if (pairing_deadline_ms_ && millis() > pairing_deadline_ms_) {
        pairing_deadline_ms_ = 0;
    }
}

bool EspNowLink::enter_pairing(uint32_t window_ms) {
    pairing_deadline_ms_ = millis() + window_ms;
    LOGI("Pairing window opened for %lu ms", static_cast<unsigned long>(window_ms));
    return true;
}

bool EspNowLink::is_pairing() const {
    return pairing_deadline_ms_ != 0;
}

bool EspNowLink::add_peer(const uint8_t *mac, const char *asset_id) {
    if (!mac || !asset_id) {
        return false;
    }
    if (!is_pairing() && !esp_now_is_peer_exist(mac)) {
        LOGW("Pairing closed, reject peer");
        return false;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 6;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = true;
    memcpy(peer.lmk, config::kEspNowLocalKey, 16);

    if (esp_now_is_peer_exist(mac)) {
        esp_now_del_peer(mac);
    }

    esp_err_t err = esp_now_add_peer(&peer);
    if (err != ESP_OK) {
        LOGE("Failed add peer err=%d", err);
        return false;
    }
    store_peer(mac, asset_id);
    if (peer_event_handler_) {
        peer_event_handler_(mac, true);
    }
    return true;
}

bool EspNowLink::remove_peer(const uint8_t *mac) {
    if (!mac) {
        return false;
    }
    esp_now_del_peer(mac);
    forget_peer(mac);
    if (peer_event_handler_) {
        peer_event_handler_(mac, false);
    }
    return true;
}

bool EspNowLink::has_peer(const uint8_t *mac) const {
    if (!mac) {
        return false;
    }
    return esp_now_is_peer_exist(mac);
}

bool EspNowLink::find_asset(const uint8_t *mac, String &asset_id_out) const {
    if (!mac) {
        return false;
    }
    for (const auto &entry : routing_) {
        if (entry.used && memcmp(entry.mac, mac, 6) == 0) {
            asset_id_out = entry.asset;
            return true;
        }
    }
    return false;
}

bool EspNowLink::resolve_mac_for_asset(const String &asset_id, uint8_t *mac_out) const {
    if (!mac_out) {
        return false;
    }
    for (const auto &entry : routing_) {
        if (entry.used && asset_id.equalsIgnoreCase(entry.asset)) {
            memcpy(mac_out, entry.mac, 6);
            return true;
        }
    }
    return false;
}

void EspNowLink::flush_routing() {
    for (auto &entry : routing_) {
        entry.used = false;
        memset(entry.mac, 0, sizeof(entry.mac));
        memset(entry.asset, 0, sizeof(entry.asset));
    }
    persist_routing();
}

void EspNowLink::handle_recv(const uint8_t *mac, const uint8_t *incomingData, int len, int rssi) {
    if (!receive_handler_ || !mac || len <= 0) {
        return;
    }
    Frame frame;
    memcpy(frame.mac, mac, 6);
    frame.length = static_cast<size_t>(len);
    frame.rssi = rssi;
    memcpy(frame.payload, incomingData, frame.length);
    frame.crc = payload::crc32(frame.payload, frame.length);
    receive_handler_(frame);
}

void EspNowLink::handle_send_status(const uint8_t *mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        LOGW("esp-now send status=%d", status);
    }
}

bool EspNowLink::load_routing() {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, true)) {
        return false;
    }
    String json = prefs.getString(kPrefsKey, "");
    prefs.end();
    if (!json.length()) {
        return true;
    }
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        LOGW("routing deserialize fail: %s", err.c_str());
        return false;
    }
    JsonArray arr = doc["peers"].as<JsonArray>();
    if (arr.isNull()) {
        return true;
    }
    size_t idx = 0;
    for (JsonObject obj : arr) {
        if (idx >= config::kMaxPeers) {
            break;
        }
        const char *mac_str = obj["mac"].as<const char*>();
        const char *asset = obj["asset"].as<const char*>();
        if (!mac_str || !asset) {
            continue;
        }
        RoutingEntry &entry = routing_[idx++];
        entry.used = true;
        memset(entry.asset, 0, sizeof(entry.asset));
        strlcpy(entry.asset, asset, sizeof(entry.asset));
        unsigned mac_bytes[6] = {0};
        sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", &mac_bytes[0], &mac_bytes[1], &mac_bytes[2], &mac_bytes[3],
               &mac_bytes[4], &mac_bytes[5]);
        for (int i = 0; i < 6; ++i) {
            entry.mac[i] = static_cast<uint8_t>(mac_bytes[i]);
        }
    }
    return true;
}

bool EspNowLink::store_peer(const uint8_t *mac, const char *asset_id) {
    RoutingEntry *slot = nullptr;
    for (auto &entry : routing_) {
        if (!entry.used || memcmp(entry.mac, mac, 6) == 0) {
            slot = &entry;
            break;
        }
    }
    if (!slot) {
        LOGW("routing table full");
        return false;
    }
    slot->used = true;
    memcpy(slot->mac, mac, 6);
    memset(slot->asset, 0, sizeof(slot->asset));
    strlcpy(slot->asset, asset_id, sizeof(slot->asset));
    return persist_routing();
}

void EspNowLink::forget_peer(const uint8_t *mac) {
    for (auto &entry : routing_) {
        if (entry.used && memcmp(entry.mac, mac, 6) == 0) {
            entry.used = false;
            memset(entry.mac, 0, sizeof(entry.mac));
            memset(entry.asset, 0, sizeof(entry.asset));
        }
    }
    persist_routing();
}

bool EspNowLink::persist_routing() {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.createNestedArray("peers");
    char mac_buf[18];
    for (const auto &entry : routing_) {
        if (!entry.used) {
            continue;
        }
        JsonObject obj = arr.createNestedObject();
        mac_to_str(entry.mac, mac_buf, sizeof(mac_buf));
        obj["mac"] = mac_buf;
        obj["asset"] = entry.asset;
    }
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, false)) {
        return false;
    }
    String json;
    serializeJson(doc, json);
    bool ok = prefs.putString(kPrefsKey, json) > 0;
    prefs.end();
    return ok;
}

const RoutingEntry *EspNowLink::routing_table(size_t &count) const {
    count = sizeof(routing_) / sizeof(routing_[0]);
    return routing_;
}

}  // namespace espnow_link

