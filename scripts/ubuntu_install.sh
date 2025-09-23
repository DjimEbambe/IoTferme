#!/usr/bin/env bash
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
  echo "Ce script doit être exécuté en root (sudo)." >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y ca-certificates curl gnupg lsb-release

# Node.js 20
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs build-essential

# Python & outils
apt-get install -y python3 python3-venv python3-pip git unzip make rsync

# Nginx
apt-get install -y nginx-full

# Redis
apt-get install -y redis-server

# Mosquitto
apt-get install -y mosquitto mosquitto-clients

# MongoDB Community
if ! dpkg -l | grep -q mongodb-org; then
  curl -fsSL https://pgp.mongodb.com/server-6.0.asc | gpg --dearmor -o /usr/share/keyrings/mongodb-server-6.0.gpg
  echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-6.0.gpg ] https://repo.mongodb.org/apt/ubuntu $(lsb_release -cs)/mongodb-org/6.0 multiverse" >/etc/apt/sources.list.d/mongodb-org-6.0.list
  apt-get update
  apt-get install -y mongodb-org
fi
systemctl enable --now mongod

# InfluxDB 2
if ! dpkg -l | grep -q influxdb2; then
  curl -fsSL https://repos.influxdata.com/influxdata-archive_compat.key | gpg --dearmor -o /usr/share/keyrings/influxdata-archive-keyring.gpg
  echo "deb [ signed-by=/usr/share/keyrings/influxdata-archive-keyring.gpg ] https://repos.influxdata.com/debian stable stable" >/etc/apt/sources.list.d/influxdata.list
  apt-get update
  apt-get install -y influxdb2
fi
systemctl enable --now influxdb

# UFW
apt-get install -y ufw
ufw allow OpenSSH
ufw allow 80/tcp
ufw enable <<<"y"

# Utilisateur applicatif
if ! id -u farmapp >/dev/null 2>&1; then
  useradd -m -s /bin/bash farmapp
fi
mkdir -p /opt/farmstack/{web-gateway,iot-streamer,ml-engine} /var/log/farmstack
chown -R farmapp:farmapp /opt/farmstack /var/log/farmstack

systemctl enable redis-server
systemctl enable mosquitto

cat <<MSG
Installation de base terminée.
- Node $(node -v)
- Python $(python3 --version)
Placez le dépôt dans /opt/farmstack puis lancez scripts/bootstrap_vps.sh.
MSG
