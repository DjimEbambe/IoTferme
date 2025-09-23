#pragma once

#include <Arduino.h>
#include <ModbusMaster.h>

namespace sensors {

struct PzemReadings {
    float voltage_v = NAN;
    float current_a = NAN;
    float power_w = NAN;
    float energy_Wh = NAN;
    float pf = NAN;
    float frequency = NAN;
};

class PzemModbus {
  public:
    bool begin(HardwareSerial &serial, uint8_t addr = 0x01, uint32_t baud = 9600, int8_t de_re_pin = -1,
               int8_t rx_pin = -1, int8_t tx_pin = -1);
    bool read(PzemReadings &out);
    bool reset_energy();
    void set_address(uint8_t addr);

  private:
    bool sync_transaction(uint16_t start, uint16_t count, uint16_t *dest);

    ModbusMaster node_;
    HardwareSerial *serial_ = nullptr;
    int8_t de_re_pin_ = -1;
};

}  // namespace sensors

