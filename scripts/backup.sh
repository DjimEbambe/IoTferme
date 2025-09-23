#!/usr/bin/env bash
set -euo pipefail

BACKUP_ROOT=${1:-/opt/farmstack/backups}
TIMESTAMP=$(date +%Y%m%d%H%M)
WORKDIR="$BACKUP_ROOT/$TIMESTAMP"
GPG_RECIPIENT=${GPG_RECIPIENT:-}

mkdir -p "$WORKDIR"

mongodump --uri "${MONGO_URI:-mongodb://localhost:27017}" --out "$WORKDIR/mongo"

if command -v influx >/dev/null 2>&1; then
  influx backup "$WORKDIR/influx"
fi

mkdir -p "$WORKDIR/configs"
cp /etc/nginx/sites-available/farmstack.conf "$WORKDIR/configs" || true
cp /etc/mosquitto/mosquitto.conf "$WORKDIR/configs" || true
cp /etc/mosquitto/aclfile "$WORKDIR/configs" || true
cp /etc/systemd/system/web-gateway.service "$WORKDIR/configs" || true
cp /etc/systemd/system/iot-streamer.service "$WORKDIR/configs" || true
cp /etc/systemd/system/ml-engine.service "$WORKDIR/configs" || true

ARCHIVE="$BACKUP_ROOT/farmstack-$TIMESTAMP.tar.gz"
tar -czf "$ARCHIVE" -C "$BACKUP_ROOT" "$TIMESTAMP"
rm -rf "$WORKDIR"

echo "Backup créé : $ARCHIVE"

if [[ -n "$GPG_RECIPIENT" ]]; then
  gpg --yes --encrypt --recipient "$GPG_RECIPIENT" "$ARCHIVE"
  echo "Archive chiffrée : $ARCHIVE.gpg"
fi
