# Architecture Générale IoTferme

## Vue d'ensemble

L'écosystème IoTferme s'articule autour de quatre couches principales :

| Couche | Matériel / Logiciel | Rôle principal | Protocoles clés |
| --- | --- | --- | --- |
| **Cloud** | `web-gateway`, `iot-streamer`, `ml-engine`, MQTT broker (TLS) | Collecte centralisée, supervision, analytics & ML | HTTPS (REST/WebSocket), MQTT/TLS (QoS1) |
| **Edge** | Raspberry Pi 5 (edge-agent FastAPI + systemd) | Agrégation locale, résilience offline, UI terrain | USB CDC (COBS/MsgPack), MQTT/TLS, HTTP(S) |
| **Passerelle** | ESP32-C6 (`gateway_simple_c6`) | Pont ESP-NOW ↔ USB, enrichissement RSSI, routage commandes | ESP-NOW, USB CDC 921 600 bps, MessagePack |
| **Capteurs / Actionneurs** | ESP32-S3/C6 (T-RelayS3, modules capteurs) | Mesure environnementale, contrôle relais | ESP-NOW (PMK/LMK), MessagePack/JSON |

## Flux de données

1. **Devices → Passerelle**
   - Protocole : ESP-NOW (payload JSON/MessagePack, clé PMK/LMK partagée).
   - Données : télémétries, événements, ACK.
   - Atouts : latence <10 ms, consommation minimisée, résilient au bruit Wi-Fi.

2. **Passerelle → Edge**
   - Protocole : USB CDC 921 600 bps, trames `COBS(payload + CRC16)`.
   - Payload : MessagePack (fallback CBOR).
   - Rôle : enrichissement (MAC, RSSI), commandes `cmd`, `cfg:set_mac`, `time_sync`.

3. **Edge → Cloud**
   - Protocole : MQTT/TLS (QoS 1) avec backlog SQLite.
   - Topics : `v1/farm/{site}/{device}/telemetry/*`, `/cmd`, `/ack`, `/status`.
   - Résilience : `BacklogManager` rejoue dès reconnexion réseau.

4. **UI / Diagnostics**
   - Interface : FastAPI + Jinja2 (`/`, `/diag`, `/commands`).
   - API : `POST /api/diag/set-mac`, `POST /api/cmd`, `POST /api/diag/time-sync`, etc.
   - Sécurité : Basic Auth optionnelle (`edge_basic_auth_*`), exposition LAN contrôlée.

## Composants

### Cloud
- **MQTT Broker** (TLS, CA système, identifiants `.env`) : pivot pub/sub pour télémétries, commandes, ACK, statut.
- **`web-gateway`** (cluster web) :
  - Fournit APIs REST/WebSocket, UI supervision.
  - Ingestion MQTT → HTTP push, partages avec clients externes.
  - Protocoles : HTTPS (REST), WS (temps réel), MQTT client interne.
- **`iot-streamer`** :
  - Service Node.js qui consomme les topics MQTT, alimente bases/time-series (InfluxDB, etc.).
  - Exporte flux temps réel (WebSocket) et triggers d’alertes.
- **`ml-engine`** :
  - Microservice Python (FastAPI) hébergeant modèles ML (prédiction, détection d’anomalies).
  - Reçoit flux via `iot-streamer`/MQTT, renvoie insights vers MQTT (`/status`, `/event`).
- Observabilité : Grafana/Influx/Prometheus (hors scope), LWT (`status`) pour détecter agents offline.

### Edge (Raspberry Pi 5)
- Projet `edge-device-stack/edge-agent` (FastAPI, asyncio).
- Services internes :
  - `SerialBridge`: USB ↔ asyncio (codec configurable `serial_codec`).
  - `MQTTClient`: TLS, publish QoS1, reconnexion automatique.
  - `BacklogManager`: queue SQLite (`storage.py`).
  - `CommandManager`: retries, timeouts, enregistre ACK.
  - `ESPNowRouter`: suivi des pairs (MAC, RSSI, firmware).
  - UI locale + API diag (`templates/`, `static/js/app.js`).
- Déploiement via systemd (`build/edge-agent.service`), udev (`udev/` -> `/dev/ttyESP-GW`).

### Passerelle (ESP32-C6)
- Firmware `arduino_alt/gateway_simple_c6/gateway_simple_c6.ino`.
- Fonctionnalités :
  - Table de pairs ESP-NOW, auto-enregistrement MAC/asset.
  - Gestion `cfg:set_mac` (persist NVS, reboot automatique).
  - Propagation `time_sync`, `pair_begin/end`, `ping`.
  - Encodage MessagePack et CRC16.

### Capteurs / Actionneurs
- Exemple `arduino_alt/device_sensor_simple_s3` (T-RelayS3) :
  - Support `ENABLE_REAL_SENSORS` (SHT41, lux analogique, MQ135) et `ENABLE_RELAY_CONTROL` (6 relais).
  - Mode synthétique si capteurs absents (`real off`).
  - Commandes `type:feature` pour activer/désactiver capteurs/relays à chaud.
- Autres firmwares (PZEM, etc.) utilisent la même chaîne ESP-NOW → Gateway → Edge.

## Choix d'architecture
- **Edge local unique** : Raspberry Pi 5, service systemd (pas de Docker) ⇒ overhead minimal, admin simplifiée.
- **Passerelle ESP32-C6** : isole le réseau ESP-NOW du LAN, conversion MsgPack/USB rapide.
- **Capteurs ESP32** : firmware modulable, protocole léger (ESP-NOW) adapté aux environnements agricoles.
- **Backhaul MQTT** : standard industriel, compatibilité cloud existant, support QoS et backlog.

## Recommandations opérationnelles
- **Systemd** : `Restart=always`, `After=network-online.target`, journal via journald.
- **Logs** : définir `LOG_DIR` (ou laisser journald) pour éviter problèmes de rotation.
- **Sécurité** : clés ESP-NOW (PMK/LMK) uniques site, TLS obligatoire pour MQTT, Basic Auth UI si exposée hors réseau interne.
- **Maintenance passerelle** : scripts `arduino_alt/tools`, udev pour raccourci `/dev/ttyESP-GW`.
- **CI/CD** : build sketches via `arduino-cli`/PlatformIO, edge-agent via `make install` + tests (`pytest`).
