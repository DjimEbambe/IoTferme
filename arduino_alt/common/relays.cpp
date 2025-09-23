#include "relays.h"

#include "config.h"

namespace relays {

bool RelayBank::begin(const uint8_t *pins, size_t count) {
    count_ = min(count, static_cast<size_t>(kRelayCount));
    for (size_t i = 0; i < count_; ++i) {
        entries_[i].pin = pins[i];
        entries_[i].enabled = pins[i] != 0xFF;
        entries_[i].state = false;
        entries_[i].last_change = millis();
        if (entries_[i].enabled) {
            pinMode(entries_[i].pin, OUTPUT);
            digitalWrite(entries_[i].pin, LOW);
        }
    }
    last_command_ms_ = millis();
    return true;
}

void RelayBank::loop() {
    unsigned long now = millis();
    if (now - last_command_ms_ > RELAY_FAILSAFE_TIMEOUT_MS) {
        force_all_off();
        last_command_ms_ = now;
    }
}

bool RelayBank::command(RelayIndex index, bool on) {
    if (index >= kRelayCount || index >= count_) {
        return false;
    }
    Entry &entry = entries_[index];
    if (!entry.enabled) {
        return false;
    }
    unsigned long now = millis();
    uint32_t since_change = now - entry.last_change;
    if (entry.state == on) {
        last_command_ms_ = now;
        return true;
    }
    if (on) {
        if (since_change < config::kRelayMinOffMs) {
            return false;
        }
    } else {
        if (since_change < config::kRelayMinOnMs) {
            return false;
        }
    }
    digitalWrite(entry.pin, on ? HIGH : LOW);
    entry.state = on;
    entry.last_change = now;
    last_command_ms_ = now;
    return true;
}

bool RelayBank::get(RelayIndex index) const {
    if (index >= kRelayCount || index >= count_) {
        return false;
    }
    const Entry &entry = entries_[index];
    return entry.enabled && entry.state;
}

void RelayBank::force_all_off() {
    for (size_t i = 0; i < count_; ++i) {
        if (!entries_[i].enabled) {
            continue;
        }
        digitalWrite(entries_[i].pin, LOW);
        entries_[i].state = false;
        entries_[i].last_change = millis();
    }
}

RelayStatus RelayBank::status() const {
    RelayStatus status = {};
    for (size_t i = 0; i < count_; ++i) {
        status.states[i] = entries_[i].enabled && entries_[i].state;
    }
    return status;
}

void RelayBank::touch() {
    last_command_ms_ = millis();
}

}  // namespace relays

