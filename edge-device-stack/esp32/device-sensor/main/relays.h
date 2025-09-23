#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RELAY_CHANNEL_LAMP = 0,
    RELAY_CHANNEL_FAN,
    RELAY_CHANNEL_HEATER,
    RELAY_CHANNEL_DOOR,
    RELAY_CHANNEL_EXTRA1,
    RELAY_CHANNEL_EXTRA2,
} relay_channel_t;

esp_err_t relays_init(void);
esp_err_t relays_set(relay_channel_t channel, bool on);
void relays_all_off(void);

#ifdef __cplusplus
}
#endif
