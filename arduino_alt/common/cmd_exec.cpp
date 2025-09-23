#include "cmd_exec.h"

#include "debug_log.h"
#include "time_sync.h"

namespace cmd_exec {

bool apply(const payload::CommandMessage &cmd, relays::RelayBank &relays, Setpoints &setpoints_out, payload::AckPayload &ack_out) {
    bool overall_ok = true;
    if (cmd.relay.has_lamp) {
        overall_ok &= relays.command(relays::kLamp, cmd.relay.lamp_on);
    }
    if (cmd.relay.has_fan) {
        overall_ok &= relays.command(relays::kFan, cmd.relay.fan_on);
    }
    if (cmd.relay.has_heater) {
        overall_ok &= relays.command(relays::kHeater, cmd.relay.heater_on);
    }
    if (cmd.relay.has_door) {
        overall_ok &= relays.command(relays::kDoor, cmd.relay.door_open);
    }
    if (cmd.relay.has_aux1) {
        overall_ok &= relays.command(relays::kAux1, cmd.relay.aux1_on);
    }
    if (cmd.relay.has_aux2) {
        overall_ok &= relays.command(relays::kAux2, cmd.relay.aux2_on);
    }

    if (!isnan(cmd.setpoints.t_c)) {
        setpoints_out.t_c = cmd.setpoints.t_c;
    }
    if (!isnan(cmd.setpoints.rh)) {
        setpoints_out.rh = cmd.setpoints.rh;
    }
    if (!isnan(cmd.setpoints.lux)) {
        setpoints_out.lux = cmd.setpoints.lux;
    }

    ack_out.asset_id = cmd.asset_id;
    ack_out.correlation_id = cmd.correlation_id;
    ack_out.ok = overall_ok;
    ack_out.message = overall_ok ? F("applied") : F("partial_failure");
    ack_out.timestamp_iso = time_sync::now_iso();

    return overall_ok;
}

}  // namespace cmd_exec

