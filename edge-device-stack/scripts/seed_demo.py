#!/usr/bin/env python3
"""Seed demo data into edge-agent SQLite store."""
from __future__ import annotations

import asyncio
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SRC_PATH = REPO_ROOT / "edge-agent" / "src"
if str(SRC_PATH) not in sys.path:
    sys.path.insert(0, str(SRC_PATH))

from storage import Storage, StorageConfig  # type: ignore

DEFAULT_DB = Path("/var/lib/edge-agent/edge.db")


async def main_async(db_path: Path) -> None:
    storage = Storage(StorageConfig(db_path, retention_days=28))
    await storage.connect()
    now = datetime.now(timezone.utc)
    payload = {
        "type": "telemetry",
        "site": "KIN-GOLIATH",
        "device": "esp32gw-01",
        "asset_id": "A-PP-01",
        "channel": "env",
        "metrics": {"t_c": 27.5, "rh": 61.0, "lux": 120},
        "ts": now.isoformat(),
        "msg_id": "seed-1",
        "idempotency_key": "seed-1",
    }
    await storage.put_backlog(
        now,
        "v1/farm/KIN-GOLIATH/esp32gw-01/telemetry/env",
        json.dumps(payload),
        1,
        "seed-1",
    )
    await storage.close()
    print(f"Seeded demo data into {db_path}")


if __name__ == "__main__":
    asyncio.run(main_async(DEFAULT_DB))
