#include "debug_log.h"

#include <stdarg.h>

namespace {
uint8_t g_level = DEBUG_LOG_LEVEL;
bool g_started = false;
}  // namespace

namespace debug_log {

void begin(bool wait_for_usb, unsigned long baud) {
    if (!g_started) {
        Serial.begin(baud);
        if (wait_for_usb) {
            unsigned long start = millis();
            while (!Serial && millis() - start < 2000) {
                delay(10);
            }
        }
        g_started = true;
    }
}

void set_level(uint8_t level) {
    g_level = level;
}

void log(uint8_t level, const char *fmt, ...) {
    if (level > g_level) {
        return;
    }
    if (!g_started) {
        begin(false);
    }
    static char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf_P(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Serial.println(buffer);
}

void hexdump(uint8_t level, const uint8_t *data, size_t len) {
    if (level > g_level || !data || len == 0) {
        return;
    }
    if (!g_started) {
        begin(false);
    }
    for (size_t i = 0; i < len; ++i) {
        if (i % 16 == 0) {
            Serial.printf("%04x: ", static_cast<unsigned>(i));
        }
        Serial.printf("%02x ", data[i]);
        if (i % 16 == 15 || i + 1 == len) {
            Serial.println();
        }
    }
}

}  // namespace debug_log

