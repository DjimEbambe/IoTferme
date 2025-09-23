#include "espnow_init.h"

#include <string.h>

#include "config.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

static const char *TAG = "espnow";
static uint8_t s_default_lmk[ESP_NOW_KEY_LEN] = {0};

esp_err_t espnow_setup(const uint8_t *pmk, const uint8_t *lmk, int channel)
{
    ESP_RETURN_ON_ERROR(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE), TAG, "set channel");
    ESP_RETURN_ON_ERROR(esp_now_init(), TAG, "esp_now_init");
    if (pmk) {
        uint8_t pmk_key[ESP_NOW_KEY_LEN] = {0};
        memcpy(pmk_key, pmk, ESP_NOW_KEY_LEN);
        ESP_RETURN_ON_ERROR(esp_now_set_pmk(pmk_key), TAG, "set pmk");
    }
    if (lmk) {
        memset(s_default_lmk, 0, sizeof(s_default_lmk));
        memcpy(s_default_lmk, lmk, ESP_NOW_KEY_LEN);
    } else {
        memset(s_default_lmk, 0, sizeof(s_default_lmk));
        memcpy(s_default_lmk, EDGE_ESPNOW_LMK, strnlen(EDGE_ESPNOW_LMK, ESP_NOW_KEY_LEN));
    }
    ESP_LOGI(TAG, "esp-now ready on channel %d", channel);
    return ESP_OK;
}

esp_err_t espnow_add_peer(const uint8_t *mac)
{
    esp_now_peer_info_t info = {
        .channel = EDGE_ESPNOW_CHANNEL,
        .ifidx = WIFI_IF_STA,
        .encrypt = true,
    };
    memcpy(info.peer_addr, mac, ESP_NOW_ETH_ALEN);
    memset(info.lmk, 0, ESP_NOW_KEY_LEN);
    memcpy(info.lmk, s_default_lmk, ESP_NOW_KEY_LEN);
    ESP_RETURN_ON_ERROR(esp_now_add_peer(&info), TAG, "add peer");
    return ESP_OK;
}

esp_err_t espnow_remove_peer(const uint8_t *mac)
{
    return esp_now_del_peer(mac);
}

esp_err_t espnow_send(const uint8_t *mac, const uint8_t *data, size_t len, bool encrypt)
{
    (void)encrypt;
    return esp_now_send(mac, data, len);
}

void espnow_register_recv(esp_now_recv_cb_t cb)
{
    esp_now_register_recv_cb(cb);
}
