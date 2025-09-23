#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float lux;
} light_sample_t;

esp_err_t light_sensor_init(void);
esp_err_t light_sensor_read(light_sample_t *sample);

#ifdef __cplusplus
}
#endif
