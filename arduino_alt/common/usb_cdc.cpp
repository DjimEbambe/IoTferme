#include "usb_cdc.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "debug_log.h"

namespace usb_cdc {

namespace {

SemaphoreHandle_t queue_mutex;
Frame frame_queue[4];
size_t queue_head = 0;
size_t queue_tail = 0;
size_t queue_count = 0;

uint8_t rx_buffer[config::kUsbFrameMax];
size_t rx_index = 0;

size_t cobs_encode(const uint8_t *input, size_t length, uint8_t *output, size_t max_len) {
    size_t read_index = 0;
    size_t write_index = 1;
    uint8_t code = 1;

    while (read_index < length) {
        if (input[read_index] == 0) {
            output[write_index - code] = code;
            code = 1;
            write_index++;
            if (write_index >= max_len) {
                return 0;
            }
            read_index++;
        } else {
            output[write_index++] = input[read_index++];
            code++;
            if (code == 0xFF) {
                output[write_index - code] = code;
                code = 1;
                if (write_index >= max_len) {
                    return 0;
                }
            }
        }
        if (write_index >= max_len) {
            return 0;
        }
    }
    output[write_index - code] = code;
    return write_index;
}

size_t cobs_decode(const uint8_t *input, size_t length, uint8_t *output, size_t max_len) {
    size_t read_index = 0;
    size_t write_index = 0;

    while (read_index < length) {
        uint8_t code = input[read_index];
        if (code == 0 || read_index + code > length + 1) {
            return 0;
        }
        read_index++;
        for (uint8_t i = 1; i < code; ++i) {
            if (write_index >= max_len) {
                return 0;
            }
            output[write_index++] = input[read_index++];
        }
        if (code < 0xFF && read_index < length) {
            if (write_index >= max_len) {
                return 0;
            }
            output[write_index++] = 0;
        }
    }
    return write_index;
}

void push_frame(const uint8_t *payload, size_t length) {
    if (!queue_mutex) {
        return;
    }
    if (length == 0 || length > config::kUsbFrameMax) {
        return;
    }
    if (xSemaphoreTake(queue_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    if (queue_count == (sizeof(frame_queue) / sizeof(frame_queue[0]))) {
        queue_tail = (queue_tail + 1) % (sizeof(frame_queue) / sizeof(frame_queue[0]));
        queue_count--;
    }
    Frame &slot = frame_queue[queue_head];
    memcpy(slot.data, payload, length);
    slot.length = length;
    queue_head = (queue_head + 1) % (sizeof(frame_queue) / sizeof(frame_queue[0]));
    queue_count++;
    xSemaphoreGive(queue_mutex);
}

bool pop_frame(Frame &out) {
    if (!queue_mutex) {
        return false;
    }
    if (xSemaphoreTake(queue_mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return false;
    }
    if (queue_count == 0) {
        xSemaphoreGive(queue_mutex);
        return false;
    }
    Frame &slot = frame_queue[queue_tail];
    memcpy(out.data, slot.data, slot.length);
    out.length = slot.length;
    queue_tail = (queue_tail + 1) % (sizeof(frame_queue) / sizeof(frame_queue[0]));
    queue_count--;
    xSemaphoreGive(queue_mutex);
    return true;
}

}  // namespace

void begin(unsigned long baud, bool wait_for_host) {
    if (!queue_mutex) {
        queue_mutex = xSemaphoreCreateMutex();
    }
    Serial.begin(baud);
    if (wait_for_host) {
        unsigned long start = millis();
        while (!Serial && millis() - start < 2000) {
            delay(10);
        }
    }
}

void loop() {
    while (Serial.available()) {
        uint8_t byte_read = Serial.read();
        if (byte_read == 0) {
            size_t decoded = cobs_decode(rx_buffer, rx_index, rx_buffer, sizeof(rx_buffer));
            if (decoded > 0) {
                push_frame(rx_buffer, decoded);
            } else {
                LOGW("USB frame decode error");
            }
            rx_index = 0;
        } else {
            if (rx_index < sizeof(rx_buffer)) {
                rx_buffer[rx_index++] = byte_read;
            } else {
                rx_index = 0;
            }
        }
    }
}

bool send(const uint8_t *payload, size_t length) {
    if (!payload || length == 0 || length > config::kUsbFrameMax - 2) {
        return false;
    }
    static uint8_t buffer[config::kUsbFrameMax + 4];
    size_t encoded = cobs_encode(payload, length, buffer, sizeof(buffer) - 1);
    if (!encoded) {
        return false;
    }
    buffer[encoded++] = 0x00;
    size_t written = Serial.write(buffer, encoded);
    Serial.flush();
    return written == encoded;
}

bool available() {
    if (!queue_mutex) {
        return false;
    }
    bool result = false;
    if (xSemaphoreTake(queue_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        result = queue_count > 0;
        xSemaphoreGive(queue_mutex);
    }
    return result;
}

bool pop(Frame &out) {
    return pop_frame(out);
}

void flush() {
    Serial.flush();
}

}  // namespace usb_cdc

