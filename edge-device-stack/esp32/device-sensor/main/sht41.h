#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature_c;
    float humidity_pct;
} sht41_sample_t;

esp_err_t sht41_init(void);
esp_err_t sht41_read(sht41_sample_t *out_sample);

#ifdef __cplusplus
}
#endif
