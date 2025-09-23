#include "nvs_store.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "nvs_store";
static nvs_handle_t s_handle = 0;

esp_err_t nvs_store_init(const char *namespace_name)
{
    esp_err_t rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rc = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rc);
    rc = nvs_open(namespace_name, NVS_READWRITE, &s_handle);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed %s", esp_err_to_name(rc));
        return rc;
    }
    return ESP_OK;
}

esp_err_t nvs_store_set_float(const char *key, float value)
{
    if (s_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t rc = nvs_set_blob(s_handle, key, &value, sizeof(value));
    if (rc == ESP_OK) {
        rc = nvs_commit(s_handle);
    }
    return rc;
}

esp_err_t nvs_store_get_float(const char *key, float *out_value, float default_value)
{
    if (s_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t required = sizeof(float);
    esp_err_t rc = nvs_get_blob(s_handle, key, out_value, &required);
    if (rc == ESP_OK && required == sizeof(float)) {
        return ESP_OK;
    }
    *out_value = default_value;
    return rc == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : rc;
}

esp_err_t nvs_store_set_blob(const char *key, const void *data, size_t size)
{
    if (s_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t rc = nvs_set_blob(s_handle, key, data, size);
    if (rc == ESP_OK) {
        rc = nvs_commit(s_handle);
    }
    return rc;
}

esp_err_t nvs_store_get_blob(const char *key, void *data, size_t *size)
{
    if (s_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_get_blob(s_handle, key, data, size);
}
