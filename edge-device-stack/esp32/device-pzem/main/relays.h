#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RELAY_PUMP = 0,
    RELAY_VALVE,
} pzem_relay_t;

esp_err_t pzem_relays_init(void);
esp_err_t pzem_relays_set(pzem_relay_t relay, bool on);
void pzem_relays_off(void);

#ifdef __cplusplus
}
#endif
