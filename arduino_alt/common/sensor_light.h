#pragma once

#include <Arduino.h>

namespace sensors {

class LightSensor {
  public:
    void begin(uint8_t pin);
    float read_lux(uint8_t oversample = 0) const;

  private:
    uint8_t pin_ = 0;
};

}  // namespace sensors

