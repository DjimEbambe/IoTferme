from __future__ import annotations

from datetime import datetime, timedelta, timezone

import pytest


@pytest.mark.asyncio
async def test_store_telemetry_and_latest(storage):
    ts = datetime.now(timezone.utc)
    await storage.store_telemetry(
        ts=ts,
        asset_id="A-PP-01",
        metrics={"t_c": 28.5, "rh": 62.1},
        rssi_dbm=-60,
    )
    await storage.store_telemetry(
        ts=ts + timedelta(seconds=5),
        asset_id="A-PP-01",
        metrics={"mq135_ppm": 120},
        rssi_dbm=-58,
    )
    latest = await storage.latest_telemetry()
    assert latest
    asset = latest[0]
    assert asset["asset_id"] == "A-PP-01"
    assert "t_c" in asset["metrics"]
    assert asset["rssi_dbm"] == -58


@pytest.mark.asyncio
async def test_backlog_roundtrip(storage):
    ts = datetime.now(timezone.utc)
    ids = []
    for i in range(3):
        ids.append(
            await storage.put_backlog(
                ts=ts + timedelta(seconds=i),
                topic="v1/farm/KIN-GOLIATH/esp32gw-01/telemetry/env",
                payload_json="{}",
                qos=1,
                idempotency_key=f"key-{i}",
            )
        )
    stats = await storage.backlog_counts()
    assert stats["queued"] == 3
    rows = await storage.fetch_backlog()
    assert len(rows) == 3
    await storage.mark_sent([row[0] for row in rows], acked=True)
    purged = await storage.purge_backlog()
    assert purged >= 0
    stats_after = await storage.backlog_counts()
    assert stats_after["queued"] == 0
