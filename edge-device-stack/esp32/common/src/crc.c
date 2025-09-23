#include "crc.h"

uint16_t crc16_ccitt(const uint8_t *data, size_t len, uint16_t initial)
{
    uint16_t crc = initial;
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
