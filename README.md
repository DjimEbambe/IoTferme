# IotFarm — Gestion connectée de la ferme

Bienvenue dans la plateforme **IotFarm**. Ce dépôt rassemble tout le nécessaire pour piloter une exploitation avicole : portail web, passerelles IoT, moteur ML et scripts d’exploitation.

## Composants principaux

- `web-gateway/` – Portail Express + EJS, jumeau numérique 3D (Three.js), API ERP, Socket.IO.
- `iot-streamer/` – Worker Node 20 (MQTT ↔ Redis ↔ Influx) avec validation Ajv et corrélation d’ACK.
- `ml-engine/` – FastAPI (Python 3.11) pour détection d’anomalies, prévisions, optimisation de ration.
- `nginx/`, `mosquitto/`, `influx/`, `systemd/` – Configuration prête pour la prod.
- `docs/` – Guides (dont `docs/mongodb_plan.md` pour le schéma MongoDB).
- `scripts/` – Installation, bootstrap VPS, seed, backup/restauration, simulateur MQTT.

Visite rapide : `docs/mongodb_plan.md` décrit le modèle MongoDB et l’architecture (graphes Mermaid pour Sites, Bâtiments, Points de production, Twin, etc.).

## Prérequis

- Ubuntu 22.04 LTS (accès sudo).
- Node.js 20, Python 3.11, MongoDB 6.x, InfluxDB 2.x, Redis 7, Mosquitto 2.
- Domain ou IP publique si déploiement complet.

## Installation express

```bash
sudo ./scripts/ubuntu_install.sh           # dépendances OS
cp .env.example .env                        # secrets racine (adapter)
cp web-gateway/.env.example web-gateway/.env
sudo ./scripts/bootstrap_vps.sh            # déploie web/iot/ml + nginx + systemd
```

Pour recharge manuelle :

```bash
sudo systemctl enable --now web-gateway iot-streamer ml-engine
sudo systemctl status web-gateway
```

Développement local :

```bash
make dev        # lance web-gateway (nodemon) + iot-streamer + ml-engine
npm run watch:client --prefix web-gateway   # rebuild bundles front
```

### Seeds et données de démo

```bash
./scripts/seed_dev.sh   # insère site, bâtiments A/B, PPs, assets, twin de base
```

## Tester et vérifier

```bash
npm run lint --prefix web-gateway   # ESLint (globals browser configurées)
make test                           # Mocha/Chai + Pytest
```

Endpoints utiles :

- `http://localhost:8000/` – Portail IotFarm (twin 3D, ERP, dashboard).
- `http://localhost:8000/healthz` – ping web.
- `http://127.0.0.1:8002/ml/health` (+ header `X-API-Key`) – health ML Engine.
- MQTT Mosquitto : `mqtt://localhost:1883` (TLS possible, ACL par device).

## Sauvegarder / restaurer

```bash
./scripts/backup.sh /opt/iotfarm/backups
./scripts/restore.sh /opt/iotfarm/backups/iotfarm-YYYYmmddHHMM.tar.gz
```

## Allers plus loin

- Diagrammes PlantUML dans `plantuml/` (architecture, modèle de données).
- Monitoring conseillé : Prometheus/Grafana + exports Mosquitto/Mongo/Influx.
- Auth : RBAC intégré, compatible avec un futur SSO (OIDC).

## Ressources complémentaires

- `docs/mongodb_plan.md` – plan MongoDB (collections, seeds, plugins).
- `web-gateway/docs/` – documentation front/twin.
- `scripts/` – outillage DevOps (installation, updates, simulateurs, backups).

Pour toute contribution : créer une branche, ajouter tests et lancer `npm run lint` / `make test` avant PR.

Bonne exploration et bienvenue sur **IotFarm** !
