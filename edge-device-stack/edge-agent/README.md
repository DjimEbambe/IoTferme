# Edge Agent (Raspberry Pi 5)

Application FastAPI assurant le lien entre la passerelle ESP32 (USB CDC) et le back-end MQTT. Elle fournit une UI locale (FastAPI + Jinja2) pour le diagnostic, un stockage SQLite, un moteur de backlog et des commandes terrains.

## Démarrage rapide

```bash
cd edge-device-stack/edge-agent
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
uvicorn src.main:app --reload --host 0.0.0.0 --port 8081
```

> Les scripts `make install|start|logs` dans `edge-device-stack/` automatisent l’installation système (systemd, udev, dossiers).

## Flux USB ↔ Passerelle

- Encapsulation : `COBS(payload + CRC16)` terminée par `0x00`
- Payload : MessagePack (`serializeMsgPack` côté ESP32, `msgpack` côté Python)
- Redondance : fallback CBOR si `serial_codec=cbor`
- Cadence : 921 600 bps, `SerialConfig` configurable via `.env` (`USB_DEVICE`, `SERIAL_CODEC`, etc.)

## UI locale

- `/` : statut global (dernier télémetrie, backlog, health checks)
- `/commands` : envoi manuel de commandes JSON (relay/test-relay)
- `/buffer` : inspection et purge du backlog outbound
- `/diag` : actions de maintenance (time-sync, pairing mode, reset backlog, ping) + **nouvelle commande “Enregistrer MAC”**

### Configuration MAC passerelle

La section “Actions” de `/diag` propose un formulaire :

1. Saisir l’adresse MAC cible (format `02:11:22:33:44:55`).
2. Option “Persister (NVS)” conserve l’adresse dans la NVS de la passerelle.
3. Après validation, l’agent envoie `{"type":"cfg","op":"set_mac"}` via MessagePack → la passerelle confirme et redémarre pour appliquer.

Les événements renvoyés par la passerelle (`set_mac`, `mac_loaded`) apparaissent dans `/diag` via le tableau “Devices” et sont aussi publiés sur MQTT (topic `.../status`).

## Points clés

- Stockage SQLite dans `/var/lib/edge-agent/edge.db`
- Backlog appliqué si MQTT indisponible (`queue_out`)
- Scheduler APScheduler (cron purge quotidienne, time-sync périodique)
- SerialBridge reconnecte automatiquement le `/dev/ttyESP-GW`

Voir `../docs/frames.md` et `../docs/edge-flow.md` pour le contrat de données complet.
