#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EDGE_DIR="$REPO_DIR/edge-agent"
VENV_DIR="$EDGE_DIR/.venv"
SERVICE_USER="${SERVICE_USER:-$(id -un)}"
SERVICE_GROUP="${SERVICE_GROUP:-$(id -gn)}"
DATA_DIR="${DATA_DIR:-/var/lib/edge-agent}"
LOG_DIR="${LOG_DIR:-/var/log/edge-agent}"
LOG_FILE="$LOG_DIR/edge-agent.log"

sudo apt-get update
sudo apt-get install -y python3-venv python3-pip mosquitto-clients sqlite3

python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"

pip install --upgrade pip
pip install -e "$EDGE_DIR"

sudo mkdir -p "$DATA_DIR"
sudo chown "$SERVICE_USER":"$SERVICE_GROUP" "$DATA_DIR"

sudo mkdir -p "$LOG_DIR"
sudo touch "$LOG_FILE"
sudo chown "$SERVICE_USER":"$SERVICE_GROUP" "$LOG_DIR" "$LOG_FILE"

if [ ! -f "$EDGE_DIR/.env" ]; then
  cp "$EDGE_DIR/.env.example" "$EDGE_DIR/.env"
fi

deactivate

echo "Edge agent environment ready. Use 'make enable' then 'make start'."
