#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ppm;
    float r0;
} mq135_sample_t;

esp_err_t mq135_init(float r0_default);
esp_err_t mq135_read(float temperature_c, float humidity_pct, mq135_sample_t *sample);

#ifdef __cplusplus
}
#endif
