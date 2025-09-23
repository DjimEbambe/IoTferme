#pragma once

#include <stdint.h>
#include <time.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void time_sync_init(void);
void time_sync_set_offset(int64_t offset_ms);
int64_t time_sync_get_offset(void);
int64_t time_sync_now_ms(void);
esp_err_t time_sync_handle_frame(const uint8_t *payload, size_t length);

#ifdef __cplusplus
}
#endif
