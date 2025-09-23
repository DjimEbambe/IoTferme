# ESP32-C6 Gateway (ESP-NOW ↔ USB CDC demo)

Firmware Arduino/ESP-IDF minimal destiné à la passerelle ESP32-C6 utilisée avec le Raspberry Pi 5 (edge-agent). Il illustre la conversion ESP-NOW ⇆ USB CDC avec MessagePack + COBS et la gestion distante de la MAC.

## Build & Flash

1. Ouvrir le sketch dans l’IDE Arduino ou PlatformIO.
2. Placer la carte `ESP32C6 Dev Module` (Arduino core ≥ 3.0.0).
3. Ajuster `kSerialBaud` si nécessaire (921 600 bps par défaut).
4. Compiler & téléverser.

## Protocole USB

- Trames `COBS(payload + CRC16-CCITT)` terminées par `0x00`
- Payload MessagePack (ArduinoJson `serializeMsgPack`/`deserializeMsgPack`)
- Types émis : `telemetry`, `ack`, `event`, `status`
- Types reçus : `cmd`, `add_peer`, `list_peers`, `time_sync`, `ping`, `cfg:set_mac`

## Gestion de la MAC

- Au boot, la passerelle charge une MAC optionnelle depuis la NVS (`gw_cfg/sta_mac`) et l’applique avant `esp_now_init()`.
- Commande envoyée par l’agent :
  ```json
  {
    "type": "cfg",
    "op": "set_mac",
    "mac": "02:11:22:33:44:55",
    "persist": true
  }
  ```
- La passerelle répond via `event:set_mac` puis redémarre pour appliquer la nouvelle adresse.
- Cocher “Persister” dans `/diag` pour conserver l’adresse (NVS). Sans persistance, la MAC est utilisée pour le prochain boot uniquement.

## Pairing & Routage

- `add_peer` ajoute un périphérique à la table locale et à `esp_now_add_peer()`
- Les télémetries reçues via ESP-NOW ajoutent automatiquement l’asset/MAC à la table
- Les commandes (`cmd`) sont routées via l’asset ou le champ `mac`

## Dépannage

- Événement `crc_error` → vérifier le câblage ou le débit série
- Événement `host_decode` → MessagePack invalide (vérifier la version de l’agent)
- Événement `set_mac_invalid` → format MAC incorrect

Pour plus de détails, voir la documentation `edge-device-stack/docs/frames.md`.
