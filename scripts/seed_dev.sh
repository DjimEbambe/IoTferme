#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
export NODE_ENV=development
if [[ -f "$REPO_ROOT/web-gateway/.env" ]]; then
  set -o allexport
  # shellcheck source=/dev/null
  source "$REPO_ROOT/web-gateway/.env"
  set +o allexport
fi

cd "$REPO_ROOT/web-gateway"
npm run seed
