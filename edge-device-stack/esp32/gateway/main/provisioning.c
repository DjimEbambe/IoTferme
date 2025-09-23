#include "provisioning.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "prov";
static uint64_t s_deadline_us = 0;
static uint8_t s_last_mac[6] = {0};

void provisioning_init(void)
{
    s_deadline_us = 0;
    memset(s_last_mac, 0, sizeof(s_last_mac));
}

void provisioning_begin(uint32_t duration_s)
{
    s_deadline_us = esp_timer_get_time() + (uint64_t)duration_s * 1000000ULL;
    ESP_LOGI(TAG, "pairing window open %u s", duration_s);
}

void provisioning_end(void)
{
    s_deadline_us = 0;
    ESP_LOGI(TAG, "pairing window closed");
}

bool provisioning_is_open(void)
{
    if (s_deadline_us == 0) {
        return false;
    }
    if (esp_timer_get_time() > s_deadline_us) {
        provisioning_end();
        return false;
    }
    return true;
}

bool provisioning_allow_mac(const uint8_t mac[6])
{
    if (!provisioning_is_open()) {
        return false;
    }
    memcpy(s_last_mac, mac, 6);
    return true;
}
