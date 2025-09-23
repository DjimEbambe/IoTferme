#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16_ccitt(const uint8_t *data, size_t len, uint16_t initial);

#ifdef __cplusplus
}
#endif
