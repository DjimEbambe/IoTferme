#pragma once

#include <Arduino.h>

namespace board_detect {

enum class BoardType {
    kUnknown = 0,
    kRelayS3,
    kLoRaC6,
};

BoardType detect();
const __FlashStringHelper *to_string(BoardType type);

inline bool is_s3(BoardType type) {
    return type == BoardType::kRelayS3;
}

inline bool is_c6(BoardType type) {
    return type == BoardType::kLoRaC6;
}

}  // namespace board_detect

