#pragma once

#include <Arduino.h>

#include "opts.h"
#include "version.h"

namespace config {

constexpr char kSiteId[] = "KIN-GOLIATH";

struct DeviceIdentity {
    const char *device_id;
    const char *asset_id;
};

constexpr DeviceIdentity kGatewayIdentity{"gateway-bridge-01", "A-GW-01"};
constexpr DeviceIdentity kSensorIdentity{"relay-s3-01", "A-PP-03"};
constexpr DeviceIdentity kSensorC6Identity{"c6-sensor-01", "A-PP-06"};
constexpr DeviceIdentity kPzemIdentity{"pzem-01", "A-PZ-01"};

constexpr uint8_t kEspNowPrimaryKey[16] = {0x4d, 0x27, 0x92, 0x9c, 0x1b, 0x6a, 0x50, 0xba, 0x34, 0xa2, 0x0c, 0xee, 0x16, 0x48, 0x73, 0x5d};
constexpr uint8_t kEspNowLocalKey[16] = {0x83, 0x4a, 0x1f, 0xc2, 0x7e, 0x61, 0x59, 0xfe, 0xd1, 0x30, 0x6a, 0x33, 0x44, 0x9d, 0xbf, 0xe7};

constexpr char kDefaultPairingPassphrase[] = "kin-edge-pair";

constexpr size_t kMaxPeers = 16;
constexpr size_t kUsbFrameMax = 1024;
constexpr size_t kEspNowPayloadMax = 250;

constexpr char kTopicTelemetryEnv[] = "v1/farm/%s/%s/telemetry/env";
constexpr char kTopicTelemetryPower[] = "v1/farm/%s/%s/telemetry/power";
constexpr char kTopicTelemetryWater[] = "v1/farm/%s/%s/telemetry/water";
constexpr char kTopicTelemetryIncubator[] = "v1/farm/%s/%s/telemetry/incubator";
constexpr char kTopicCmd[] = "v1/farm/%s/%s/cmd";
constexpr char kTopicAck[] = "v1/farm/%s/%s/ack";
constexpr char kTopicStatus[] = "v1/farm/%s/%s/status";

constexpr uint32_t kTimeSyncIntervalMs = 60000;
constexpr uint32_t kTimeSyncMaxSkewMs = 2000;

constexpr uint32_t kSensorWarmupMs = 180000;  // MQ-135 warmup

// ADC characteristics
constexpr float kLightSensorMvPerLux = 1.2f;
constexpr uint8_t kAdcOversample = 8;

// MQ-135 calibration defaults
constexpr float kMq135DefaultR0 = 9.5f;
constexpr float kMq135CleanAirFactor = 3.6f;
constexpr float kMq135TempComp = 0.04f;
constexpr float kMq135RhComp = 0.03f;

// Relay safety
constexpr uint32_t kRelayMinOnMs = 3000;
constexpr uint32_t kRelayMinOffMs = 3000;

inline const char *fw_version() {
    return firmware_version();
}

}  // namespace config

