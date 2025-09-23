#include "sensors_sht41.h"

#include <Wire.h>

#include "debug_log.h"

namespace sensors {

bool Sht41Wrapper::begin(uint8_t addr) {
    (void)addr;
    if (!sensor_.begin(&Wire)) {
        LOGE("SHT41 init failed");
        return false;
    }
    sensor_.setPrecision(SHT4X_HIGH_PRECISION);
    sensor_.setHeater(SHT4X_NO_HEATER);
    return true;
}

bool Sht41Wrapper::read(float &temp_c, float &rh) {
    sensors_event_t humidity, temp;
    if (sensor_.getEvent(&humidity, &temp)) {
        temp_c = temp.temperature;
        rh = humidity.relative_humidity;
        return true;
    }
    return false;
}

}  // namespace sensors

