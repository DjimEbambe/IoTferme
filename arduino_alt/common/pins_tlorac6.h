#pragma once

#include <Arduino.h>

namespace pins_tlorac6 {

constexpr uint8_t LED_STATUS = 15;
constexpr uint8_t LED_ERROR = 16;

constexpr uint8_t I2C_SDA = 18;
constexpr uint8_t I2C_SCL = 17;

constexpr uint8_t UART485_TX = 20;
constexpr uint8_t UART485_RX = 19;
constexpr uint8_t UART485_RE = 21;
constexpr uint8_t UART485_DE = 21;

constexpr uint8_t LIGHT_SENSOR_ADC = 5;
constexpr uint8_t MQ135_ADC = 6;

constexpr uint8_t RELAY_PUMP = 2;

// LoRa SX1262 mapping (optional backup path)
constexpr uint8_t LORA_SCK = 8;
constexpr uint8_t LORA_MISO = 9;
constexpr uint8_t LORA_MOSI = 10;
constexpr uint8_t LORA_CS = 7;
constexpr uint8_t LORA_RST = 4;
constexpr uint8_t LORA_BUSY = 3;
constexpr uint8_t LORA_DIO1 = 11;

}  // namespace pins_tlorac6

