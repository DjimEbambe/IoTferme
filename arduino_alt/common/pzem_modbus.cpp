#include "pzem_modbus.h"

#include "debug_log.h"

namespace sensors {

namespace {
int g_de_re_pin = -1;

void pre_transmission() {
    if (g_de_re_pin >= 0) {
        digitalWrite(g_de_re_pin, HIGH);
    }
}

void post_transmission() {
    if (g_de_re_pin >= 0) {
        digitalWrite(g_de_re_pin, LOW);
    }
}

}  // namespace

bool PzemModbus::begin(HardwareSerial &serial, uint8_t addr, uint32_t baud, int8_t de_re_pin, int8_t rx_pin, int8_t tx_pin) {
    serial_ = &serial;
    de_re_pin_ = de_re_pin;
    if (rx_pin >= 0 && tx_pin >= 0) {
        serial_->begin(baud, SERIAL_8N1, rx_pin, tx_pin);
    } else {
        serial_->begin(baud, SERIAL_8N1);
    }
    if (de_re_pin_ >= 0) {
        pinMode(de_re_pin_, OUTPUT);
        digitalWrite(de_re_pin_, LOW);
        g_de_re_pin = de_re_pin_;
    }
    node_.begin(addr, serial_);
    node_.preTransmission(pre_transmission);
    node_.postTransmission(post_transmission);
    return true;
}

bool PzemModbus::read(PzemReadings &out) {
    uint16_t buffer[10] = {0};
    if (!sync_transaction(0x0000, 10, buffer)) {
        return false;
    }
    out.voltage_v = buffer[0] / 10.0f;
    uint32_t current_raw = (static_cast<uint32_t>(buffer[1]) << 16) | buffer[2];
    out.current_a = current_raw / 1000.0f;
    uint32_t power_raw = (static_cast<uint32_t>(buffer[3]) << 16) | buffer[4];
    out.power_w = power_raw / 10.0f;
    uint32_t energy_raw = (static_cast<uint32_t>(buffer[5]) << 16) | buffer[6];
    out.energy_Wh = static_cast<float>(energy_raw);
    out.frequency = buffer[7] / 10.0f;
    out.pf = buffer[8] / 100.0f;
    return true;
}

bool PzemModbus::reset_energy() {
    uint8_t result = node_.writeSingleRegister(0x0002, 0x0001);
    return result == node_.ku8MBSuccess;
}

void PzemModbus::set_address(uint8_t addr) {
    node_.setSlave(addr);
}

bool PzemModbus::sync_transaction(uint16_t start, uint16_t count, uint16_t *dest) {
    uint8_t result = node_.readInputRegisters(start, count);
    if (result != node_.ku8MBSuccess) {
        LOGW("Modbus read fail %u", result);
        return false;
    }
    for (uint16_t i = 0; i < count; ++i) {
        dest[i] = node_.getResponseBuffer(i);
    }
    return true;
}

}  // namespace sensors

