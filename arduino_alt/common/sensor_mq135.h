#pragma once

#include <Arduino.h>

namespace sensors {

class Mq135Sensor {
  public:
    bool begin(uint8_t pin, const char *nvs_key = "mq135_r0");
    float read_ppm(float temp_c, float rh);
    bool is_warmup_done() const;
    void set_r0(float r0);
    float get_r0() const;
    bool save_r0();

  private:
    float read_resistance() const;
    void load_r0();

    uint8_t pin_ = 0;
    String nvs_key_;
    float r0_ = 10.0f;
    uint32_t warmup_start_ms_ = 0;
};

}  // namespace sensors

