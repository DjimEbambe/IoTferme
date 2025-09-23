#include "sensor_mq135.h"

#include <Preferences.h>

#include "config.h"
#include "debug_log.h"

namespace sensors {

namespace {
constexpr float kLoadResistor = 10000.0f;
constexpr float kVcc = 3.3f;
}

bool Mq135Sensor::begin(uint8_t pin, const char *nvs_key) {
    pin_ = pin;
    pinMode(pin_, INPUT);
    nvs_key_ = nvs_key;
    load_r0();
    warmup_start_ms_ = millis();
    return true;
}

float Mq135Sensor::read_ppm(float temp_c, float rh) {
    float rs = read_resistance();
    if (rs <= 0.0f) {
        return NAN;
    }
    float ratio = rs / r0_;
    float ppm = 116.6020682f * powf(ratio, -2.7690349f);
    if (!isnan(temp_c)) {
        ppm *= (1.0f + config::kMq135TempComp * (temp_c - 20.0f));
    }
    if (!isnan(rh)) {
        ppm *= (1.0f + config::kMq135RhComp * (rh - 55.0f));
    }
    return ppm;
}

bool Mq135Sensor::is_warmup_done() const {
    return millis() - warmup_start_ms_ >= config::kSensorWarmupMs;
}

void Mq135Sensor::set_r0(float r0) {
    if (r0 > 0.1f && r0 < 1000.0f) {
        r0_ = r0;
    }
}

float Mq135Sensor::get_r0() const {
    return r0_;
}

bool Mq135Sensor::save_r0() {
    Preferences prefs;
    if (!prefs.begin("mq135", false)) {
        return false;
    }
    bool ok = prefs.putFloat(nvs_key_.c_str(), r0_) == sizeof(float);
    prefs.end();
    return ok;
}

float Mq135Sensor::read_resistance() const {
    float mv = analogReadMilliVolts(pin_);
    float v = mv / 1000.0f;
    if (v <= 0.0f) {
        return -1.0f;
    }
    float rs = (kVcc * kLoadResistor / v) - kLoadResistor;
    return rs;
}

void Mq135Sensor::load_r0() {
    Preferences prefs;
    if (prefs.begin("mq135", true)) {
        float stored = prefs.getFloat(nvs_key_.c_str(), config::kMq135DefaultR0);
        if (stored > 0.1f && stored < 1000.0f) {
            r0_ = stored;
        }
        prefs.end();
    }
}

}  // namespace sensors

