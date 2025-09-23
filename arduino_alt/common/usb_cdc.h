#pragma once

#include <Arduino.h>

#include "config.h"

namespace usb_cdc {

struct Frame {
    uint8_t data[config::kUsbFrameMax];
    size_t length = 0;
};

void begin(unsigned long baud = 115200, bool wait_for_host = true);
void loop();
bool send(const uint8_t *payload, size_t length);
bool available();
bool pop(Frame &out);
void flush();

}  // namespace usb_cdc

