#pragma once

#include <Adafruit_SHT4x.h>

namespace sensors {

class Sht41Wrapper {
  public:
    bool begin(uint8_t addr = 0x44);
    bool read(float &temp_c, float &rh);

  private:
    Adafruit_SHT4x sensor_;
};

}  // namespace sensors

