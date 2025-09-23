#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
ENV_FILE="$REPO_ROOT/.env"
if [[ -f "$ENV_FILE" ]]; then
  set -o allexport
  # shellcheck source=/dev/null
  source "$ENV_FILE"
  set +o allexport
fi

: "${FARMSTACK_DOMAIN:=farmstack.local}"
: "${NGINX_TLS_ENABLED:=false}"
: "${ML_API_KEY:=change-me}"

if [[ $EUID -ne 0 ]]; then
  echo "Exécuter scripts/bootstrap_vps.sh avec sudo." >&2
  exit 1
fi

install_service() {
  local name=$1
  local src=$2
  local dest=$3
  mkdir -p "$(dirname "$dest")"
  cp "$src" "$dest"
  chown farmapp:farmapp "$dest"
}

mkdir -p /opt/farmstack
rsync -a --delete "$REPO_ROOT/web-gateway/" /opt/farmstack/web-gateway
rsync -a --delete "$REPO_ROOT/iot-streamer/" /opt/farmstack/iot-streamer
rsync -a --delete "$REPO_ROOT/ml-engine/" /opt/farmstack/ml-engine
chown -R farmapp:farmapp /opt/farmstack

for service in web-gateway iot-streamer ml-engine; do
  if [[ -f "$REPO_ROOT/$service/.env" ]]; then
    cp "$REPO_ROOT/$service/.env" "/opt/farmstack/$service/.env"
    chown farmapp:farmapp "/opt/farmstack/$service/.env"
  fi
done

cd /opt/farmstack/web-gateway && sudo -u farmapp npm install
cd /opt/farmstack/iot-streamer && sudo -u farmapp npm install --omit=dev
cd /opt/farmstack/web-gateway && sudo -u farmapp npm run build:client
cd /opt/farmstack/web-gateway && sudo -u farmapp npm prune --omit=dev

cd /opt/farmstack/ml-engine
if [[ ! -d venv ]]; then
  sudo -u farmapp python3 -m venv venv
fi
sudo -u farmapp ./venv/bin/pip install --upgrade pip
sudo -u farmapp ./venv/bin/pip install -r requirements.txt

# Nginx
mkdir -p /etc/nginx/ssl/farmstack
cp "$REPO_ROOT/nginx/sites-available/farmstack.conf" /etc/nginx/sites-available/farmstack.conf
cp "$REPO_ROOT/nginx/snippets/ssl-params.conf" /etc/nginx/snippets/ssl-params.conf
ln -sf /etc/nginx/sites-available/farmstack.conf /etc/nginx/sites-enabled/farmstack.conf

if [[ "$NGINX_TLS_ENABLED" == "true" ]]; then
  if [[ -d "$REPO_ROOT/nginx/ssl" ]]; then
    cp -r "$REPO_ROOT/nginx/ssl/"* /etc/nginx/ssl/farmstack/
  fi
  ufw allow 443/tcp || true
fi
systemctl reload nginx

# Mosquitto
cp "$REPO_ROOT/mosquitto/mosquitto.conf" /etc/mosquitto/mosquitto.conf
cp "$REPO_ROOT/mosquitto/aclfile" /etc/mosquitto/aclfile
if [[ ! -f /etc/mosquitto/passwd ]]; then
  touch /etc/mosquitto/passwd
  mosquitto_passwd -b /etc/mosquitto/passwd farm_iot "${MQTT_PASSWORD:-farmstack}" || true
  mosquitto_passwd -b /etc/mosquitto/passwd streamer "${MQTT_PASSWORD:-farmstack}" || true
fi
systemctl restart mosquitto

# InfluxDB tasks
if command -v influx >/dev/null 2>&1; then
  influx setup --skip-verify --bucket iot_raw --org "${INFLUX_ORG:-farmstack}" --username admin --password "${INFLUX_TOKEN:-token}" --force || true
  influx bucket create --name iot_agg_1m --org "${INFLUX_ORG:-farmstack}" || true
  influx bucket create --name iot_agg_5m --org "${INFLUX_ORG:-farmstack}" || true
  influx task create --org "${INFLUX_ORG:-farmstack}" --file "$REPO_ROOT/influx/tasks/downsample_1m.flux" --token "${INFLUX_TOKEN:-token}" || true
  influx task create --org "${INFLUX_ORG:-farmstack}" --file "$REPO_ROOT/influx/tasks/downsample_5m.flux" --token "${INFLUX_TOKEN:-token}" || true
fi

# Mongo indexes + user
if command -v mongosh >/dev/null 2>&1; then
  if [[ -n "${MONGO_USER:-}" && -n "${MONGO_PASSWORD:-}" ]]; then
    mongosh --quiet <<MONGO
use farmstack
if (!db.getUser("${MONGO_USER}")) {
  db.createUser({user: "${MONGO_USER}", pwd: "${MONGO_PASSWORD}", roles: [{role: "readWrite", db: "farmstack"}]})
}
MONGO
  fi
  mongosh --quiet <<'MONGO'
use farmstack
db.site.createIndex({site_id: 1}, {unique: true})
db.buildings.createIndex({building_id: 1}, {unique: true})
db.zones.createIndex({zone_id: 1}, {unique: true})
db.pp.createIndex({pp_id: 1}, {unique: true})
db.lots.createIndex({lot_id: 1}, {unique: true})
MONGO
fi

# Systemd
install_service web-gateway "$REPO_ROOT/systemd/web-gateway.service" /etc/systemd/system/web-gateway.service
install_service iot-streamer "$REPO_ROOT/systemd/iot-streamer.service" /etc/systemd/system/iot-streamer.service
install_service ml-engine "$REPO_ROOT/systemd/ml-engine.service" /etc/systemd/system/ml-engine.service
systemctl daemon-reload
systemctl enable web-gateway.service iot-streamer.service ml-engine.service

# Logrotate
cat >/etc/logrotate.d/farmstack <<'ROTATE'
/var/log/farmstack/*.log {
  weekly
  missingok
  rotate 12
  compress
  notifempty
  copytruncate
}
ROTATE

systemctl restart web-gateway.service || true
systemctl restart iot-streamer.service || true
systemctl restart ml-engine.service || true

echo "Bootstrap terminé pour ${FARMSTACK_DOMAIN}."
