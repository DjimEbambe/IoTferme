#pragma once

#include "payload.h"
#include "relays.h"

namespace cmd_exec {

struct Setpoints {
    float t_c = NAN;
    float rh = NAN;
    float lux = NAN;
};

bool apply(const payload::CommandMessage &cmd, relays::RelayBank &relays, Setpoints &setpoints_out, payload::AckPayload &ack_out);

}  // namespace cmd_exec

