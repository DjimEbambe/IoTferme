# FarmStack — Plateforme micro-services pour ferme avicole

Solution complète pour piloter une ferme avicole moderne à Kinshasa : ERP MongoDB, jumeau numérique 3D, ingestion IoT temps réel, KPI, moteur ML et déploiement natif (Nginx + systemd).

mac c6 1 skotch : 
mac s3 : 48:ca:43:36:e7:0c
mac c6 2 : 40:4c:ca:5c:50:74

## Contenu du dépôt

- `web-gateway/` — Portail web (Express 5, EJS SSR, Socket.IO, MongoDB, InfluxDB, MQTT commands) + RBAC & sécurité.
- `iot-streamer/` — Worker Node 20 gérant MQTT ↔ Redis ↔ Influx avec validation Ajv, corrélateur ACK et replay.
- `ml-engine/` — API FastAPI (Python 3.11) pour anomalies, prévisions, optimisation de ration, scheduler APS.
- `nginx/`, `mosquitto/`, `influx/` — Configurations prêtes pour la prod.
- `scripts/` — Automatisation (installation Ubuntu, bootstrap, seeds, backups, simulateur MQTT).
- `systemd/` — Units pour web-gateway, iot-streamer, ml-engine.
- `plantuml/` — Diagrammes architecture & modèle de données.

## Prérequis

- Ubuntu 22.04 LTS (root/sudo).
- Node.js 20, Python 3.11, MongoDB 6.0, InfluxDB 2.x, Redis 7, Mosquitto 2.
- Accès shell avec `sudo` pour installer services et systemd.

## Installation

1. **Provisioning des dépendances**
   ```bash
   sudo ./scripts/ubuntu_install.sh
   ```
2. **Configurer l'environnement**
   - Copier `.env.example` → `.env` à la racine et dans chaque micro-service (`web-gateway/.env`, ...).
   - Renseigner mots de passe (Mongo, Redis, MQTT, Influx token, ML API key, secrets sessions, etc.).
3. **Bootstrap serveur**
   ```bash
   sudo ./scripts/bootstrap_vps.sh
   ```
   Ce script :
   - Déploie le code dans `/opt/farmstack/{web-gateway,iot-streamer,ml-engine}`.
   - Installe dépendances Node/Python, configure Nginx, Mosquitto, Influx, systemd, logrotate.
   - Crée buckets Influx (iot_raw, iot_agg_1m, iot_agg_5m) + tasks Flux.
4. **(Optionnel) Seeds & données de démonstration**
   ```bash
   cp web-gateway/.env.example web-gateway/.env # adapter credentials
   ./scripts/seed_dev.sh
   ```
5. **Services systemd**
   ```bash
   sudo systemctl enable --now web-gateway iot-streamer ml-engine
   sudo systemctl status web-gateway
   ```

## Utilisation

- Portail web : `http(s)://<votre-domaine>/` (sessions Redis, RBAC, CSRF, CSP).
- Endpoints API :
  - `/api/kpi`, `/api/series`, `/api/cmd`, `/api/erp/*`, `/api/ml/*` (proxy ML)
  - `/healthz` (web), `/ml/health` (FastAPI, header `X-API-Key`).
- WebSocket Socket.IO (`/ws`) : rooms par scope (site/bâtiment/zone/asset). Events : `telemetry.update`, `kpi.update`, `incident.new`, `cmd.ack`.
- Jumeau numérique 3D (Babylon.js) : layout modulaire, commandes IoT, replayer 24h.

## Tests & Qualité

```bash
make test         # Mocha/Chai/Sinon (web-gateway, iot-streamer) + Pytest (ml-engine)
make lint         # ESLint + Ruff
npm run watch:client --prefix web-gateway   # rebuild front bundles live
```

Couverture cible ≥ 80 %. Tests unitaires fournis :
- KPI service, API commandes, rooms (web-gateway)
- Corrélateur ACK (iot-streamer)
- Anomalies, ration, API health (ml-engine)

## Sécurité

- Helmet + CSP stricte (nonces), HSTS si TLS, rate limiting global.
- Sessions sur Redis, cookies `Secure/HttpOnly/SameSite=Lax`.
- CSRF via header `csrf-token` (pages & API mutantes).
- Validation Ajv/Joi sur payloads, audit trail (commandes & ERP).
- MQTT Mosquitto : ACL par device, QoS1, support TLS manuel.
- Nginx : reverse proxy, gzip/Brotli, support Real-IP Cloudflare (voir script commenté).

## SLO & Observabilité

- Télémétrie MQTT → UI p95 ≤ 2 s (Redis pub/sub + Socket.IO).
- Commande UI → ACK p95 ≤ 3 s (corrélation + retries).
- Replayer 24h (Influx agg 1m/5m) ≤ 2 s.
- Healthchecks : `/healthz`, `/ml/health`, ping Redis/Mongo/Influx/Mosquitto dans scripts.
- Logs JSON (pino) vers `/var/log/farmstack/*.log`, rotation hebdo.

## Sauvegarde & restauration

```bash
./scripts/backup.sh /opt/farmstack/backups
./scripts/restore.sh /opt/farmstack/backups/farmstack-YYYYmmddHHMM.tar.gz
```
Option GPG : exporter `GPG_RECIPIENT` avant backup.

## Déploiements sans docker

- Zero-downtime : déployer sous `/opt/farmstack/releases/<timestamp>` puis mettre à jour symlink et `systemctl reload` (exemple dans README).
- TLS manuel (pas de Certbot) : déposer `fullchain.crt`/`privkey.key` dans `/etc/nginx/ssl/farmstack/` puis activer le bloc 443.

## Diagrammes

- `plantuml/container.puml` — architecture globale (Nginx, web, iot-worker, ml, DB/brokers).
- `plantuml/er_model.puml` — entités ERP (site, building, zone, PP, lot, incubation, asset, resource).

## Front web (Webpack)

- Sources UI : `web-gateway/src/frontend/**/*` (JS modules + SCSS).
- Bundles générés via Webpack 5 (`npm run build:client`), manifest servi côté SSR pour charger `main.[hash].js/css`.
- Mode développement : `npm run watch:client --prefix web-gateway` + `make up` (watch nodemon + webpack).

Génération : `make diagram` (affiche instructions PlantUML).

## Monitoring & operations

- `scripts/simulate_mqtt.js` : simulateur MQTT pour tests locaux.
- Checks : `curl http://localhost:8000/healthz`, `curl -H "X-API-Key: ..." http://127.0.0.1:8002/ml/health`.
- UFW : 22/tcp, 80/tcp, 443/tcp (optionnel).
- Toutes les réponses HTTP incluent un `X-Request-ID` (et JSON structuré `{error, code, request_id}`) pour le suivi incident.

## Conventions & outillage

- Node.js ESM ("type": "module") + lint/prettier via npm.
- Logs JSON Pino corrélés, sessions Redis sécurisées.
- Python formaté Black + lint Ruff.
- Secrets chargés depuis `.env`, stockage temps en UTC (UI en Africa/Kinshasa).

## Roadmap suggérée

1. Brancher télémétrie live (MQTT devices) + calibrations.
2. Ajuster modèles ML avec données réelles (Influx).
3. Durcir auth (OIDC/SSO), audit centralisé (ELK), CI/CD GitHub Actions.
4. Superviser via Prometheus/Grafana (exporters Mosquitto/Mongo/Influx).
