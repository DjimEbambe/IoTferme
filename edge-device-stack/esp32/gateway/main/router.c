#include "router.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_store.h"

typedef struct {
    bool used;
    uint8_t mac[6];
    char asset_id[ROUTER_ASSET_ID_MAX];
    int8_t last_rssi;
    time_t last_seen;
} router_entry_t;

static const char *TAG = "router";
static router_entry_t s_entries[ROUTER_MAX_ENTRIES];

static void load_from_nvs(void)
{
    size_t size = sizeof(s_entries);
    if (nvs_store_get_blob("router", s_entries, &size) != ESP_OK) {
        memset(s_entries, 0, sizeof(s_entries));
    }
}

esp_err_t router_init(void)
{
    ESP_RETURN_ON_ERROR(nvs_store_init("router"), TAG, "nvs init");
    load_from_nvs();
    return ESP_OK;
}

static router_entry_t *find_slot(const uint8_t mac[6])
{
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (s_entries[i].used && memcmp(s_entries[i].mac, mac, 6) == 0) {
            return &s_entries[i];
        }
    }
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (!s_entries[i].used) {
            return &s_entries[i];
        }
    }
    return NULL;
}

esp_err_t router_register(const uint8_t mac[6], const char *asset_id, bool persistent)
{
    router_entry_t *slot = find_slot(mac);
    if (!slot) {
        return ESP_ERR_NO_MEM;
    }
    slot->used = true;
    memcpy(slot->mac, mac, 6);
    strlcpy(slot->asset_id, asset_id, sizeof(slot->asset_id));
    slot->last_seen = time(NULL);
    if (persistent) {
        ESP_RETURN_ON_ERROR(nvs_store_set_blob("router", s_entries, sizeof(s_entries)), TAG, "store");
    }
    ESP_LOGI(TAG, "pair %02x:%02x:%02x:%02x:%02x:%02x -> %s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], asset_id);
    return ESP_OK;
}

const char *router_resolve(const uint8_t mac[6])
{
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (s_entries[i].used && memcmp(s_entries[i].mac, mac, 6) == 0) {
            return s_entries[i].asset_id;
        }
    }
    return NULL;
}

bool router_lookup_asset(const char *asset_id, uint8_t mac_out[6])
{
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (s_entries[i].used && strncmp(s_entries[i].asset_id, asset_id, ROUTER_ASSET_ID_MAX) == 0) {
            memcpy(mac_out, s_entries[i].mac, 6);
            return true;
        }
    }
    return false;
}

void router_touch(const uint8_t mac[6], int8_t rssi)
{
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (s_entries[i].used && memcmp(s_entries[i].mac, mac, 6) == 0) {
            s_entries[i].last_seen = time(NULL);
            s_entries[i].last_rssi = rssi;
            return;
        }
    }
}

esp_err_t router_save(void)
{
    return nvs_store_set_blob("router", s_entries, sizeof(s_entries));
}

void router_snapshot_json(char *buffer, size_t length)
{
    int written = snprintf(buffer, length, "[");
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES && written < (int)length; ++i) {
        if (!s_entries[i].used) {
            continue;
        }
        const router_entry_t *entry = &s_entries[i];
        written += snprintf(
            buffer + written,
            length - written,
            "{\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"asset_id\":\"%s\",\"rssi\":%d,\"last_seen\":%lld},",
            entry->mac[0], entry->mac[1], entry->mac[2], entry->mac[3], entry->mac[4], entry->mac[5],
            entry->asset_id,
            entry->last_rssi,
            (long long)entry->last_seen
        );
    }
    if (written > 1 && buffer[written - 1] == ',') {
        buffer[written - 1] = ']';
    } else if (written < (int)length) {
        buffer[written++] = ']';
    }
    if (written < (int)length) {
        buffer[written] = '\0';
    } else {
        buffer[length - 1] = '\0';
    }
}

void router_for_each(router_iter_cb_t cb, void *ctx)
{
    for (size_t i = 0; i < ROUTER_MAX_ENTRIES; ++i) {
        if (!s_entries[i].used) {
            continue;
        }
        if (!cb(s_entries[i].mac, s_entries[i].asset_id, ctx)) {
            break;
        }
    }
}
