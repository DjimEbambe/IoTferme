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
- `src/public/` : assets statiques (dist Webpack, modèles GLB).    k

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
