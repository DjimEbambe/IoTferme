# Flash & Build Notes (Arduino IDE)

## Core & Board Packages
- Install **ESP32 by Espressif Systems** core (≥ 3.0.0) via Boards Manager.
- Required boards:
  - `T-RelayS3` (select `ESP32S3 Dev Module` with custom settings below).
  - `T-LoRaC6` (select `ESP32C6 Dev Module`).

## Common IDE Settings
- Programmer: `USB CDC On Boot` = Enabled.
- PSRAM: Enabled (for ESP32-S3).
- Flash Frequency: 80 MHz.
- Flash Mode: QIO.
- CPU Frequency: 240 MHz (S3), 160 MHz (C6 default).
- Build Partitions:
  - ESP32-S3: `Huge APP (3MB No OTA/1MB SPIFFS)`.
  - ESP32-C6: `Factory app, two OTA partitions (2MB with OTA/1MB SPIFFS)`.
- Filesystem: `LittleFS` (run **Tools → ESP32 Sketch Data Upload** after first flash to provision storage).

## Upload Parameters
- USB CDC Only (no UART auto-reset cable required).
- Baud rate: 921 600 for upload, Serial monitor at **115 200**.
- For ESP32-S3, hold `BOOT` if auto-programming fails; release when upload starts.

## Project Notes
- `gateway_espnow_usb` targets either T-RelayS3 or T-LoRaC6 but ships S3-optimised task pinning.
- `device_sensor` and `device_pzem` auto-detect the MCU (`CONFIG_IDF_TARGET_…`), adjust pin muxing, and share the same ESP-NOW credentials.
- Ensure MAC pairing before enabling telemetry: use the CLI in `tools/enroll_cli.md` from the edge (Pi5) to authorise devices.

## LittleFS Partitioning
- Recommended minimum FS size: 1 MiB (stores ~4 weeks ring buffer at ~12 kB/day per node).
- Wipe FS (`LittleFS.format()`) only on first deployment; subsequent updates preserve backlog.

