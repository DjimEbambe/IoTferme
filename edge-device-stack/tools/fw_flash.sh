#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <gateway|sensor|pzem> [idf-args...]" >&2
  exit 1
fi

COMPONENT=$1
shift || true

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IDF_ARGS=($@)

case "$COMPONENT" in
  gateway)
    APP_DIR="$REPO_DIR/esp32/gateway"
    TARGET="esp32s3"
    ;;
  sensor)
    APP_DIR="$REPO_DIR/esp32/device-sensor"
    TARGET="esp32c6"
    ;;
  pzem)
    APP_DIR="$REPO_DIR/esp32/device-pzem"
    TARGET="esp32s3"
    ;;
  *)
    echo "Unknown component: $COMPONENT" >&2
    exit 1
    ;;
 esac

idf.py -C "$APP_DIR" set-target "$TARGET"
idf.py -C "$APP_DIR" build "${IDF_ARGS[@]}"
idf.py -C "$APP_DIR" flash "${IDF_ARGS[@]}"
