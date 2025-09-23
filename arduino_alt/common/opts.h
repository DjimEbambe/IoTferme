#pragma once

#include <stdint.h>

#ifndef USE_MSGPACK
#define USE_MSGPACK 1
#endif

#if USE_MSGPACK
#define USE_JSON 0
#else
#define USE_JSON 1
#endif

#ifndef ENABLE_LORA_BACKUP
#define ENABLE_LORA_BACKUP 0
#endif

// Core transport and retry parameters (ms)
constexpr uint32_t CMD_ACK_TIMEOUT_MS = 3000;
constexpr uint8_t CMD_RETRY_LIMIT = 2;
constexpr uint32_t PAIRING_WINDOW_MS = 120000;
constexpr uint32_t RELAY_FAILSAFE_TIMEOUT_MS = 120000;


// Feature toggles (override in build flags if needed)
#ifndef ENABLE_REAL_SENSORS
#define ENABLE_REAL_SENSORS 0
#endif

#ifndef ENABLE_RELAY_CONTROL
#define ENABLE_RELAY_CONTROL 0
#endif

#if ENABLE_REAL_SENSORS
#undef USE_FAKE_SENSORS
#define USE_FAKE_SENSORS 0
#elif !defined(USE_FAKE_SENSORS)
#define USE_FAKE_SENSORS 1
#endif

// Telemetry cadences (ms)
constexpr uint32_t TELEMETRY_ENV_PERIOD_MS_NORMAL = 10000;
constexpr uint32_t TELEMETRY_ENV_PERIOD_MS_ALARM = 2000;
constexpr uint32_t TELEMETRY_POWER_PERIOD_MS = 5000;
constexpr uint32_t TELEMETRY_WATER_PERIOD_MS = 10000;

// Storage configuration
constexpr uint32_t RINGBUF_RETENTION_DAYS = 28;
constexpr uint32_t RINGBUF_TARGET_CAPACITY_BYTES = 512 * 1024;  // 512 KiB default FS budget

// General compile-time toggles
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL 3  // 0=off,1=error,2=warn,3=info,4=debug
#endif
