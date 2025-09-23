#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float voltage_v;
    float current_a;
    float power_w;
    float energy_wh;
} pzem_sample_t;

esp_err_t pzem_init(uint8_t address);
esp_err_t pzem_read(pzem_sample_t *sample);

#ifdef __cplusplus
}
#endif
