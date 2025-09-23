#pragma once

#include <Arduino.h>

#include "opts.h"

namespace relays {

enum RelayIndex {
    kLamp = 0,
    kFan,
    kHeater,
    kDoor,
    kAux1,
    kAux2,
    kRelayCount
};

struct RelayStatus {
    bool states[kRelayCount];
};

class RelayBank {
  public:
    bool begin(const uint8_t *pins, size_t count);
    void loop();
    bool command(RelayIndex index, bool on);
    bool get(RelayIndex index) const;
    void force_all_off();
    RelayStatus status() const;
    void touch();

  private:
    struct Entry {
        uint8_t pin = 0xFF;
        bool enabled = false;
        bool state = false;
        unsigned long last_change = 0;
    };

    Entry entries_[kRelayCount];
    size_t count_ = 0;
    unsigned long last_command_ms_ = 0;
};

}  // namespace relays

