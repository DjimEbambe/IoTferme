#include "sensor_light.h"

#include "config.h"

namespace sensors {

void LightSensor::begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, INPUT);
}

float LightSensor::read_lux(uint8_t oversample) const {
    if (pin_ == 0 && pin_ != A0) {
        return NAN;
    }
    uint8_t count = oversample ? oversample : config::kAdcOversample;
    if (count == 0) {
        count = 1;
    }
    float sum_mv = 0;
    for (uint8_t i = 0; i < count; ++i) {
        sum_mv += analogReadMilliVolts(pin_);
        delayMicroseconds(200);
    }
    float mv = sum_mv / count;
    return mv / config::kLightSensorMvPerLux;
}

}  // namespace sensors

