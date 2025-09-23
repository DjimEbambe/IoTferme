#pragma once

#include <Arduino.h>

#include "opts.h"

namespace debug_log {

void begin(bool wait_for_usb = true, unsigned long baud = 115200);
void set_level(uint8_t level);
void log(uint8_t level, const char *fmt, ...);
void hexdump(uint8_t level, const uint8_t *data, size_t len);

}  // namespace debug_log

#define LOGE(fmt, ...) debug_log::log(1, PSTR(fmt), ##__VA_ARGS__)
#define LOGW(fmt, ...) debug_log::log(2, PSTR(fmt), ##__VA_ARGS__)
#define LOGI(fmt, ...) debug_log::log(3, PSTR(fmt), ##__VA_ARGS__)
#define LOGD(fmt, ...) debug_log::log(4, PSTR(fmt), ##__VA_ARGS__)

