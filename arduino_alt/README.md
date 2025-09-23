# Arduino Alt Firmware Suite

Production-ready Arduino sketches for IoTferme edge devices, separated from the ESP-IDF tree. The suite covers:
- **Gateway ESP32 (S3/C6)** – ESP-NOW ↔ USB CDC bridge to the Raspberry Pi 5 edge.
- **Device-Sensor (S3/C6)** – SHT41, light (ADC), MQ-135, six relays, LittleFS backlog.
- **Device-PZEM (S3/C6)** – PZEM-016/017 Modbus RTU reader with pump relay & storage.

All payloads adhere to the cloud topics:
- `v1/farm/{site}/{device}/telemetry/{env|power|water|incubator}`
- `v1/farm/{site}/{device}/cmd`
- `v1/farm/{site}/{device}/ack`
- `.../status` (gateway LWT via edge)

Default site/device IDs match the demo deployment (`KIN-GOLIATH`, `relay-s3-01`, `c6-sensor-01`, `pzem-01`). Firmware version is `arduino-1.0.2` (see `common/version.h`).

## Repository Layout
```
arduino_alt/
  README.md
  common/                # Shared modules (config, payloads, sensors, ESP-NOW helpers)
  gateway_espnow_usb/    # Bridge sketch
  device_sensor/         # Environment node sketch
  device_pzem/           # Power/pump node sketch
  tools/                 # Flashing & pairing docs
  examples/              # JSON command/telemetry examples
```

## Dependencies (Arduino Library Manager)
- **ArduinoJson ≥ 7.0** (JSON & MessagePack support).
- **Adafruit SHT4x** (temperature/humidity sensor).
- **ModbusMaster** (PZEM RTU client).
- **LittleFS_esp32** (if not bundled with core ≥3.0).

All other utilities are built in (`COBS`, CRC32, etc.).

## Build & Flash
- Environment node defaults to demo mode (synthetic telemetry, relays ignored). Flip the compile-time flags in `common/opts.h` to switch behaviour:
  - `ENABLE_REAL_SENSORS`: wire up SHT41 + light ADC + MQ135 on **T-RelayS3** and emit their readings.
  - `ENABLE_RELAY_CONTROL`: allow remote commands to toggle the six onboard relays.
  Both features can still be disabled at runtime via the serial console or an incoming `{"type":"feature"}` frame.

1. Install the ESP32 core (≥3.0.0) and follow `tools/flash_notes.md` for board-specific settings.
2. Select the sketch folder (`gateway_espnow_usb`, `device_sensor`, or `device_pzem`) in Arduino IDE.
3. Verify MessagePack default (`USE_MSGPACK` set in `common/opts.h`). Override with `#define USE_MSGPACK 0` at the top of a `.ino` to force JSON for debugging.
4. Upload sketch, then run **Tools → ESP32 Sketch Data Upload** to initialise LittleFS storage (once per board).

## Pairing & ESP-NOW Security
- PMK/LMK keys live in `common/config.h` (`kEspNowPrimaryKey`, `kEspNowLocalKey`). Update per deployment.
- Pairing is gated: open a 120 s window (`{"act":"pair_begin"}`) from the edge CLI, then enrol device MAC ↔ asset mapping (`tools/enroll_cli.md`).
- Whitelist, retries, and ACK tracking handled in `common/espnow_link.*`.

## Ring Buffer & Offline Retention
- `common/ringbuf_fs.*` stores MessagePack payloads per day (`LittleFS`).
- Retention: 28 days (`RINGBUF_RETENTION_DAYS`).
- Gateway and devices append before attempting uplink; backlog can be drained via a future `pull_since` command.

## CLI (USB Serial)
- **Device-Sensor:** `cal mq135`, `get setpoints`, `set setpoints <t> <rh> <lux>`, `relay lamp on`, `real on`, `real off`, `relay on`, `relay off`…
- **Device-PZEM:** `pump on/off`, `reset energy`.
- Gateway exposes pairing/time-sync controls via JSON frames; see `tools/enroll_cli.md`.

## Extending
- Toggle MessagePack/JSON using `common/opts.h` defines.
- Enable LoRa backup path (SX1262 on T-LoRaC6) by wiring RadioLib into `gateway_espnow_usb` and setting `ENABLE_LORA_BACKUP 1`.
- Update firmware version in `common/version.h` for OTA tracking.


## Simulation
- With `ENABLE_REAL_SENSORS=0` the environment node continues to emit deterministic sine-wave data (no hardware required).
- Even when compiled with real-sensor support, issue `real off` over USB (or push `{ "type": "feature", "real_sensors": false }`) to revert to synthetic mode for testing.
