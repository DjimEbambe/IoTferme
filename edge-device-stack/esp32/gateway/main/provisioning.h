#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void provisioning_init(void);
void provisioning_begin(uint32_t duration_s);
void provisioning_end(void);
bool provisioning_is_open(void);
bool provisioning_allow_mac(const uint8_t mac[6]);

#ifdef __cplusplus
}
#endif
