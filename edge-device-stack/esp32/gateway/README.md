# ESP32 Gateway Firmware

Passerelle ESP-NOW ↔ USB (CDC) pour Raspberry Pi 5. Compile avec ESP-IDF 5.x, cible par défaut ESP32-S3.

## Fonctionnalités clés
- ESPNOW chiffré (PMK/LMK) canal configurable
- Routage MAC ↔ asset_id persistant (NVS)
- Fenêtre de provisioning (pairing) déclenchée depuis l'edge-agent
- Pont USB CDC avec trames COBS + CRC16, messages JSON/CBOR (flag `USE_JSON`)
- Ack table pour suivre les `correlation_id` (timeouts/retry)
- Propagation time-sync vers les devices

## Build
```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Les composants communs sont dans `../common`.
