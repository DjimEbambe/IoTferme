# Edge Device Stack

Plateforme edge/gateway pour la ferme connectée KIN-GOLIATH. Ce dépôt regroupe l'agent Raspberry Pi 5, les firmwares ESP32 (ESP-IDF) et les outils de provisioning/maintenance cohérents avec l'application cloud existante.

## Contenu
- `edge-agent/` : application Python 3.11 (FastAPI + MQTT + SQLite + UI locale)
- `esp32/common/` : composant partagé (frames CBOR/JSON, ESPNOW, NVS, CRC)
- `esp32/gateway/` : firmware ESP32-S3 passerelle ESP-NOW ↔ USB CDC
- `esp32/device-sensor/` : firmware ESP32-C6 capteurs environnementaux + relais
- `esp32/device-pzem/` : firmware ESP32-S3 pour mesure énergétique PZEM + relais pompe
- `tools/` : scripts utilities (enroll, flash, OTA démo)
- `scripts/` : installation edge-agent, seed données
- `docs/` : documentation d'architecture, séquences et schémas PlantUML

## Identités & Conventions
- Site par défaut : `KIN-GOLIATH`
- Passerelle : `esp32gw-01`
- Devices demo : `esp32c6-01` (`A-PP-01`), `relay-s3-01`, `pzem-01`
- Topics MQTT : `v1/farm/{site}/{device}/telemetry/{env|power|water|incubator}`, `.../cmd`, `.../ack`, `.../status`
- JSON alignés cloud : `ts` ISO UTC, `idempotency_key`, `correlation_id`

## Installation Edge Agent
```bash
make install        # dépendances + venv
make enable         # systemd + udev (sudo)
make start          # démarre edge-agent
make logs           # journalctl -f edge-agent
```

Voir `docs/edge-flow.md` pour les séquences détaillées.

## Construction firmwares ESP32

Chaque sous-projet ESP-IDF se construit depuis son propre dossier (`esp32/gateway`, `esp32/device-sensor`, `esp32/device-pzem`). Pour un environnement neuf :

```bash
# Passerelle ESP32-S3 (USB CDC ↔ ESP-NOW)
cd esp32/gateway
idf.py set-target esp32s3
idf.py reconfigure        # résout les dépendances (esp_tinyusb)
idf.py build

# Capteurs ESP32-C6
cd ../device-sensor
idf.py set-target esp32c6
idf.py build

# Mesure PZEM ESP32-S3
cd ../device-pzem
idf.py set-target esp32s3
idf.py build
```

> ℹ️ La passerelle dépend du composant `espressif/esp_tinyusb` distribué via Component Manager. La commande `idf.py reconfigure` télécharge (ou met à jour) cette dépendance ; répétez-la si vous supprimez le dossier `managed_components`.

Flash : `tools/fw_flash.sh <gateway|sensor|pzem>` ou les tâches VS Code fournies dans `esp32/gateway/.vscode/`.
