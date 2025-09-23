#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t nvs_store_init(const char *namespace_name);
esp_err_t nvs_store_set_float(const char *key, float value);
esp_err_t nvs_store_get_float(const char *key, float *out_value, float default_value);
esp_err_t nvs_store_set_blob(const char *key, const void *data, size_t size);
esp_err_t nvs_store_get_blob(const char *key, void *data, size_t *size);

#ifdef __cplusplus
}
#endif
