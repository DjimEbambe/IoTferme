# Edge Flow Overview

Ce document résume les flux principaux entre les devices ESP32, la passerelle et l'agent edge exécuté sur la Raspberry Pi 5.

## Télémetrie (Device → Cloud)
1. Le device (ESP32-C6/S3) agrège les mesures capteurs (SHT41, MQ135, PZEM) toutes les 10 s (ou plus rapide si alarme).
2. La trame JSON est envoyée via ESP-NOW chiffré vers la passerelle (`gateway`).
3. La passerelle ajoute le RSSI, le MAC et pousse la trame vers l'agent via USB CDC (CBOR/COBS + CRC16).
4. L'agent edge publie le message sur MQTT (`v1/farm/KIN-GOLIATH/esp32gw-01/telemetry/{env|power|water|incubator}`) avec QoS 1.
5. En cas de coupure réseau, le message est persistant dans SQLite (`queue_out`) jusqu'au flush.

## Commande (Cloud → Device)
1. Le cloud publie une commande sur `v1/farm/KIN-GOLIATH/esp32gw-01/cmd` avec `correlation_id`.
2. L'agent edge consomme la commande (MQTT QoS1), stocke le contexte et l'envoie via USB à la passerelle.
3. La passerelle mappe `asset_id → MAC` et transmet la commande ESP-NOW. L'entrée est ajoutée dans la `ack_table` pour gérer les retries.
4. Le device applique la commande (relais/setpoints) et répond par un ACK (`ok`, `message`).
5. L'ACK remonte jusqu'au cloud via la même chaîne inverse.

## Offline & Résilience
- **MQTT down** : l'agent edge met en file (`queue_out`) les télémétries et commandes sortantes. Flush à la reconnexion (max 500 msg/s adaptatif).
- **USB down** : les commandes sont retenues et marquées `timeout` après 3 s ×2 retries. UI locale affiche l'état.
- **ESP-NOW** : Pairing limité (120 s) avec clé PMK/LMK. RSSI surveillé et exposé à l'UI.

## Système Local
- UI FastAPI (127.0.0.1:8081) rassemble statut MQTT/USB, backlog, devices et diag.
- Systemd service `edge-agent.service` (Restart=always) + journalctl.
- Udev crée `/dev/ttyESP-GW` pour l'USB CDC.
