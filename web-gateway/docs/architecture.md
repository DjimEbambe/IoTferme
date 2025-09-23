# Architecture Web Gateway

## Vue d'ensemble
- **Express 5 / Node 20 (ESM)** pour le SSR, l'API façade et la sécurité.
- **EJS + Bootstrap 5.3** pour une interface SSR responsive adaptée aux opérateurs de la ferme.
- **MongoDB (Mongoose)** pour l'ERP, les assets, lots et champs dynamiques.
- **InfluxDB 2** (via `influxdb-client`) pour les séries IoT temps réel et le replayer 24h.
- **Socket.IO** pour la diffusion WebSocket des télémétries, KPI et ACKs.
- **Redis** pour les sessions, le pub/sub en temps réel et l'intégration avec iot-streamer.
- **MQTT (Mosquitto)** pour la publication des commandes et la réception des ACKs.

## Middleware sécurité
- **Helmet** avec CSP stricte (nonce par requête), HSTS en production.
- **Sessions Redis** (`connect-redis`) avec cookies `Secure`, `HttpOnly`, `SameSite=Lax`.
- **CSRF** via `csurf` sur toutes les routes mutantes (+ header `csrf-token`).
- **Rate limiting** global (configurable via env) et audit des commandes.

## Modules principaux
- `dao/models` : schémas Mongoose incluant la gestion des `dyn_fields` validés par `FieldDef`.
- `dao/services` : services métier (KPI, ERP, Twin) encapsulant la logique d'agrégation et de workflow.
- `routes/pages` : vues SSR (Dashboard, ERP, Twin 3D) avec layout commun `layout.ejs`.
- `routes/api` : endpoints REST (`/api/kpi`, `/api/series`, `/api/cmd`, `/api/params`, `/api/erp/*`, `/api/ml/*`).
- `sockets` : configuration Socket.IO, attribution dynamique aux rooms par scope RBAC, pont Redis -> WS.
- `mqtt/publisher` : publication des commandes towards Mosquitto avec corrélation d'ACK (`EventEmitter`).

## Flux clés
1. **IoT -> UI** : MQTT `telemetry` → iot-streamer → Redis pub/sub → Socket.IO (`telemetry.update`).
2. **Commande UI -> Actif** : POST `/api/cmd` → MQTT `cmd` → device → MQTT `ack` → web-gateway / Redis → UI.
3. **KPI** : Mongoose (lots/zones/resources) + Influx agrégats (eau/kWh) → Dashboard.
4. **ML** : Proxy `/api/ml/*` ajoute `X-API-Key` et relaie vers `ml-engine` interne.

## SLO / Health
- `/healthz` exposé (JSON).
- Temps cible : MQTT → UI p95 <= 2 s, Commande → ACK p95 <= 3 s.

## Tests & Qualité
- Mocha/Chai/Sinon (`tests/*.test.js`) couvrant services KPI, API commandes et attribution WS.
- ESLint + Prettier pour la cohérence du code.
