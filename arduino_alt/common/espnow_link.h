#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <esp_idf_version.h>
#include <WiFi.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_wifi_types.h>
#endif

#include "config.h"

namespace espnow_link {

struct Frame {
    uint8_t mac[6];
    uint8_t payload[config::kEspNowPayloadMax];
    size_t length = 0;
    int16_t rssi = 0;
    uint32_t crc = 0;
};

struct AckTracker {
    bool in_use = false;
    String correlation_id;
    uint8_t mac[6] = {0};
    uint8_t retries = 0;
    uint32_t last_tx_ms = 0;
    size_t length = 0;
    uint8_t payload[config::kEspNowPayloadMax] = {0};
};

typedef void (*ReceiveHandler)(const Frame &frame);
typedef void (*AckTimeoutHandler)(const AckTracker &tracker);
typedef void (*PeerEventHandler)(const uint8_t *mac, bool added);

struct RoutingEntry {
    bool used = false;
    uint8_t mac[6] = {0};
    char asset[24] = {0};
};

class EspNowLink {
  public:
    bool begin(bool long_range = false);
    void set_receive_handler(ReceiveHandler handler);
    void set_ack_timeout_handler(AckTimeoutHandler handler);
    void set_peer_event_handler(PeerEventHandler handler);

    bool send(const uint8_t *mac, const uint8_t *payload, size_t length, const String &correlation_id = String());
    void notify_ack_received(const String &correlation_id);
    void loop();

    bool enter_pairing(uint32_t window_ms = PAIRING_WINDOW_MS);
    bool is_pairing() const;

    bool add_peer(const uint8_t *mac, const char *asset_id);
    bool remove_peer(const uint8_t *mac);
    bool has_peer(const uint8_t *mac) const;
    bool find_asset(const uint8_t *mac, String &asset_id_out) const;
    bool resolve_mac_for_asset(const String &asset_id, uint8_t *mac_out) const;

    void flush_routing();
    const RoutingEntry *routing_table(size_t &count) const;

  private:
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
    static void recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len);
#else
    static void recv_callback(const uint8_t *mac, const uint8_t *data, int len);
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static void send_callback(const wifi_tx_info_t *info, esp_now_send_status_t status);
#else
    static void send_callback(const uint8_t *mac, esp_now_send_status_t status);
#endif
    void handle_recv(const uint8_t *mac, const uint8_t *incomingData, int len, int rssi);
    void handle_send_status(const uint8_t *mac, esp_now_send_status_t status);
    bool load_routing();
    bool store_peer(const uint8_t *mac, const char *asset_id);
    void forget_peer(const uint8_t *mac);
    bool persist_routing();

    ReceiveHandler receive_handler_ = nullptr;
    AckTimeoutHandler ack_timeout_handler_ = nullptr;
    PeerEventHandler peer_event_handler_ = nullptr;

    static EspNowLink *instance_;

    AckTracker ack_slots_[config::kMaxPeers];
    RoutingEntry routing_[config::kMaxPeers];

    uint32_t pairing_deadline_ms_ = 0;
};

EspNowLink &instance();

}  // namespace espnow_link

