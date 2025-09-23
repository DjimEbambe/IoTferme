#include "time_sync.h"

#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"

static const char *TAG = "time_sync";
static int64_t s_offset_ms = 0;

void time_sync_init(void)
{
    s_offset_ms = 0;
}

void time_sync_set_offset(int64_t offset_ms)
{
    s_offset_ms = offset_ms;
}

int64_t time_sync_get_offset(void)
{
    return s_offset_ms;
}

int64_t time_sync_now_ms(void)
{
    return esp_timer_get_time() / 1000 + s_offset_ms;
}

esp_err_t time_sync_handle_frame(const uint8_t *payload, size_t length)
{
    char *buffer = strndup((const char *)payload, length);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return ESP_ERR_INVALID_ARG;
    }
    const cJSON *offset = cJSON_GetObjectItem(root, "offset_ms");
    const cJSON *epoch = cJSON_GetObjectItem(root, "epoch_ms");
    if (cJSON_IsNumber(offset)) {
        s_offset_ms = (int64_t)offset->valuedouble;
        ESP_LOGI(TAG, "offset update %lld", (long long)s_offset_ms);
    } else if (cJSON_IsNumber(epoch)) {
        int64_t now_ms = esp_timer_get_time() / 1000;
        s_offset_ms = (int64_t)epoch->valuedouble - now_ms;
        ESP_LOGI(TAG, "epoch sync offset=%lld", (long long)s_offset_ms);
    } else {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }
    cJSON_Delete(root);
    return ESP_OK;
}
