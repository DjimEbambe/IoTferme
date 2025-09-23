#pragma once

#include <Arduino.h>

namespace pins_trelay_s3 {

constexpr uint8_t RELAY_LAMP = 1;
constexpr uint8_t RELAY_FAN = 2;
constexpr uint8_t RELAY_HEATER = 3;
constexpr uint8_t RELAY_DOOR = 4;
constexpr uint8_t RELAY_AUX1 = 5;
constexpr uint8_t RELAY_AUX2 = 6;

constexpr uint8_t RELAY_PINS[6] = {
    RELAY_LAMP,
    RELAY_FAN,
    RELAY_HEATER,
    RELAY_DOOR,
    RELAY_AUX1,
    RELAY_AUX2,
};

constexpr uint8_t LED_STATUS = 38;
constexpr uint8_t LED_ERROR = 39;
constexpr uint8_t LED_PAIRING = 21;

constexpr uint8_t I2C_SDA = 8;
constexpr uint8_t I2C_SCL = 9;

constexpr uint8_t UART485_TX = 17;
constexpr uint8_t UART485_RX = 18;
constexpr uint8_t UART485_RE = 16;
constexpr uint8_t UART485_DE = 16;

constexpr uint8_t LIGHT_SENSOR_ADC = 10;
constexpr uint8_t MQ135_ADC = 11;

constexpr uint8_t USB_VBUS_SENSE = 15;

}  // namespace pins_trelay_s3

