#!/usr/bin/env bash
set -euo pipefail

ARCHIVE=${1:?"Archive .tar.gz requise"}
TMP_DIR=$(mktemp -d)

tar -xzf "$ARCHIVE" -C "$TMP_DIR"
RESTORE_PATH=$(find "$TMP_DIR" -maxdepth 1 -mindepth 1 -type d | head -n1)

if [[ -d "$RESTORE_PATH/mongo" ]]; then
  mongorestore --drop "$RESTORE_PATH/mongo"
fi

if [[ -d "$RESTORE_PATH/influx" && $(command -v influx) ]]; then
  influx restore "$RESTORE_PATH/influx"
fi

if [[ -d "$RESTORE_PATH/configs" ]]; then
  cp -r "$RESTORE_PATH/configs/." /etc/
  systemctl daemon-reload
fi

echo "Restauration terminée." 
