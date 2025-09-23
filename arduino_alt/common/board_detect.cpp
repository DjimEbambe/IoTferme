#include "board_detect.h"

#include "debug_log.h"

namespace board_detect {

BoardType detect() {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    return BoardType::kRelayS3;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    return BoardType::kLoRaC6;
#else
    return BoardType::kUnknown;
#endif
}

const __FlashStringHelper *to_string(BoardType type) {
    switch (type) {
        case BoardType::kRelayS3:
            return F("T-RelayS3");
        case BoardType::kLoRaC6:
            return F("T-LoRaC6");
        default:
            return F("Unknown");
    }
}

}  // namespace board_detect

