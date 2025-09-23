# Frames & Payloads

Toutes les trames suivent le schéma JSON aligné avec le back-end cloud. La version CBOR (optionnelle) encode exactement les mêmes clés.

## Télémetrie environnement (`channel=env`)
```json
{
  "type": "telemetry",
  "site": "KIN-GOLIATH",
  "device": "esp32gw-01",
  "asset_id": "A-PP-03",
  "channel": "env",
  "metrics": {
    "t_c": 28.6,
    "rh": 61.8,
    "mq135_ppm": 146,
    "lux": 120
  },
  "rssi_dbm": -62,
  "fw": "1.0.2",
  "msg_id": "uuid-...",
  "idempotency_key": "uuid-...",
  "ts": "2025-09-17T12:03:20.123Z"
}
```

## Commande
```json
{
  "type": "cmd",
  "asset_id": "A-PP-03",
  "relay": {"lamp": "ON", "fan": "OFF"},
  "setpoints": {"t_c": 32, "rh": 65, "lux": 150},
  "sequence": [
    {"act": "fan", "dur_s": 60},
    {"wait_s": 30},
    {"act": "heater", "dur_s": 300}
  ],
  "correlation_id": "uuid-..."
}
```

## ACK
```json
{
  "type": "ack",
  "asset_id": "A-PP-03",
  "correlation_id": "uuid-...",
  "ok": true,
  "message": "applied",
  "ts": "2025-09-17T12:03:23.100Z"
}
```

## Flux USB (CBOR/COBS + CRC16)
- Framing : `COBS(payload + CRC16)` terminé par `0x00`
- CRC16 : polynomial `0x1021`, init `0xFFFF`
- CBOR vs JSON : flag `USE_JSON` compile-time côté gateway/devices.

## TLV interne (devices)
Certaines mesures internes utilisent un format TLV léger avant conversion JSON :
- `0x01` → température (float32)
- `0x02` → humidité (float32)
- `0x03` → MQ135 ppm (float32)
- `0x10` → tension (float32)

Lors de la transmission vers la passerelle, ces TLV sont transformés en JSON/CBOR via `frames_build_*` du composant commun.
