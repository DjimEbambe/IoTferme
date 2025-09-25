# IotFarm – Portail web

Ce dossier contient le portail **Express 5 + EJS** qui expose l’interface IotFarm : dashboard, ERP, jumeau numérique (Three.js) et commandes IoT.

## Prise en main rapide

```bash
npm install
npm run lint
npm run build:client   # génère /src/public/dist
NODE_ENV=development nodemon src/server.js
```

Accès par défaut : `http://localhost:8000/` (mot de passe à configurer via `web-gateway/.env`).

## Scripts utiles

- `npm run dev` : serveur Express avec nodemon.
- `npm run build:client` : build Webpack (JS + SCSS + manifest).
- `npm run watch:client` : rebuild en continu.
- `npm run lint` : ESLint (globals navigateur pré-déclarées).

## Structure

- `src/app.js` : configuration Express (CSP, sessions Redis, CSRF, routes).
- `src/views/` : templates EJS (dashboard, ERP, twin, auth).
- `src/frontend/` : code front (modules JS + SCSS) – jumeau numérique Three.js (`modules/twinScene.js`, `modules/twin.js`).
- `src/dao/` : modèles Mongoose, services MongoDB (`docs/mongodb_plan.md` résume le schéma complet).
- `src/routes/` : API/rest pour KPI, séries, commandes, ERP.
- `src/public/` : assets statiques (dist Webpack, modèles GLB).

## Configuration

Fichier `.env` :

```
IOTFARM_ENV=development
PORT=8000
MONGO_URI=mongodb://localhost:27017/iotfarm
SESSION_SECRET=...
REDIS_URL=redis://localhost:6379
INFLUX_URL=http://localhost:8086
INFLUX_TOKEN=...
MQTT_URL=mqtt://localhost:1883
MQTT_USER=
MQTT_PASSWORD=
SOCKET_IO_KEY=...
```

Voir `src/config/env.js` pour la liste complète. Pensez à synchroniser avec les autres services (iot-streamer, ml-engine).

### Astuce multi-stades

Pour déployer sur plusieurs environnements, fournissez des variantes suffixées :

```
IOTFARM_ENV=production
MONGO_URI_PRODUCTION=mongodb://mongo-prod:27017/iotfarm
MONGO_URI_STAGING=mongodb://mongo-staging:27017/iotfarm
REDIS_URL_PRODUCTION=redis://redis-prod:6379
```

Si `IOTFARM_ENV=production`, les clés `_PRODUCTION` sont utilisées automatiquement (sinon `MONGO_URI`, `REDIS_URL`, etc.).

## Tests

```bash
npm run lint
npm test          # si besoin de lancer uniquement les tests web-gateway
```

Des seeds de démonstration (`scripts/seed_dev.sh`) insèrent Bâtiment 1 & 2, Couveuse et layout Twin.

## Documentation

- `docs/mongodb_plan.md` – modèle MongoDB (Sites, Buildings, PPs, Twin, etc.).
- `plantuml/` – diagrammes architecture / ER.
- `README.md` (racine) – vue globale IotFarm (install multi-service, scripts).

Pour contribuer, lancer `npm run lint` et `npm run build:client` avant PR.
