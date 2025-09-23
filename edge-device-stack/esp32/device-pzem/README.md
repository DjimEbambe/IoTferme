# Device PZEM Firmware

Firmware ESP32-S3 pour supervision énergétique (PZEM-016/017) et pilotage relais (pompe).

## Caractéristiques
- Modbus RTU via RS485 (UART1) pour lectures V/A/W/Wh
- Relais pompe (failsafe OFF) + support valve
- ESPNOW sécurisé vers la passerelle pour télémétrie `power`
- Ack commandes avec `correlation_id`

## Build
```bash
idf.py set-target esp32s3
idf.py build flash monitor
```
