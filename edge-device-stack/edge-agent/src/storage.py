"""SQLite storage layer (async) for telemetry, queue, events."""
from __future__ import annotations

import asyncio
import logging
from dataclasses import dataclass
from datetime import datetime, timedelta
from pathlib import Path
from typing import Iterable

import aiosqlite

LOGGER = logging.getLogger(__name__)

SCHEMA = """
PRAGMA journal_mode=WAL;
CREATE TABLE IF NOT EXISTS queue_out (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts TEXT NOT NULL,
    topic TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    qos INTEGER NOT NULL DEFAULT 1,
    sent INTEGER NOT NULL DEFAULT 0,
    acked INTEGER NOT NULL DEFAULT 0,
    idempotency_key TEXT
);
CREATE INDEX IF NOT EXISTS idx_queue_sent ON queue_out(sent, acked);

CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts TEXT NOT NULL,
    asset_id TEXT NOT NULL,
    metric TEXT NOT NULL,
    value REAL,
    quality TEXT DEFAULT 'good',
    rssi_dbm INTEGER
);
CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry(ts);
CREATE INDEX IF NOT EXISTS idx_telemetry_asset ON telemetry(asset_id);

CREATE TABLE IF NOT EXISTS ack (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts TEXT NOT NULL,
    asset_id TEXT NOT NULL,
    correlation_id TEXT NOT NULL,
    ok INTEGER NOT NULL,
    message TEXT
);
CREATE INDEX IF NOT EXISTS idx_ack_corr ON ack(correlation_id);

CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts TEXT NOT NULL,
    asset_id TEXT NOT NULL,
    type TEXT NOT NULL,
    payload_json TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_events_ts ON events(ts);
"""


@dataclass(slots=True)
class StorageConfig:
    path: Path
    retention_days: int = 28


class Storage:
    def __init__(self, config: StorageConfig) -> None:
        self._config = config
        self._lock = asyncio.Lock()
        self._db: aiosqlite.Connection | None = None

    async def connect(self) -> None:
        self._config.path.parent.mkdir(parents=True, exist_ok=True)
        self._db = await aiosqlite.connect(self._config.path)
        await self._db.executescript(SCHEMA)
        await self._db.commit()
        LOGGER.info("SQLite initialised", extra={"path": str(self._config.path)})

    async def close(self) -> None:
        if self._db:
            await self._db.close()
            LOGGER.info("SQLite connection closed")

    async def put_backlog(self, ts: datetime, topic: str, payload_json: str, qos: int, idempotency_key: str | None) -> int:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                """
                INSERT INTO queue_out (ts, topic, payload_json, qos, idempotency_key)
                VALUES (?, ?, ?, ?, ?)
                """,
                (ts.isoformat(), topic, payload_json, qos, idempotency_key),
            )
            await self._db.commit()
            return cursor.lastrowid

    async def mark_sent(self, queue_ids: Iterable[int], acked: bool = False) -> None:
        assert self._db is not None
        async with self._lock:
            await self._db.executemany(
                "UPDATE queue_out SET sent = 1, acked = ? WHERE id = ?",
                [(1 if acked else 0, queue_id) for queue_id in queue_ids],
            )
            await self._db.commit()

    async def fetch_backlog(self, limit: int = 500) -> list[tuple[int, str, str, int]]:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                """
                SELECT id, topic, payload_json, qos
                FROM queue_out
                WHERE acked = 0
                ORDER BY id ASC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [(row[0], row[1], row[2], row[3]) for row in rows]

    async def backlog_counts(self) -> dict[str, int]:
        assert self._db is not None
        async with self._lock:
            queued_cur = await self._db.execute(
                "SELECT COUNT(*) FROM queue_out WHERE acked = 0"
            )
            sent_cur = await self._db.execute(
                "SELECT COUNT(*) FROM queue_out WHERE sent = 1 AND acked = 0"
            )
            oldest_cur = await self._db.execute(
                """
                SELECT MIN(ts) FROM queue_out WHERE acked = 0
                """
            )
            queued = (await queued_cur.fetchone())[0]
            inflight = (await sent_cur.fetchone())[0]
            oldest = (await oldest_cur.fetchone())[0]
        return {
            "queued": int(queued or 0),
            "inflight": int(inflight or 0),
            "oldest_ts": oldest,
        }

    async def backlog_entries(self, limit: int = 50) -> list[dict[str, str | int | bool | None]]:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                """
                SELECT id, ts, topic, payload_json, qos, sent, acked, idempotency_key
                FROM queue_out
                WHERE acked = 0
                ORDER BY id ASC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "id": row[0],
                "ts": row[1],
                "topic": row[2],
                "payload_json": row[3],
                "qos": row[4],
                "sent": bool(row[5]),
                "acked": bool(row[6]),
                "idempotency_key": row[7],
            }
            for row in rows
        ]

    async def purge_backlog(self) -> int:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                "DELETE FROM queue_out WHERE acked = 1"
            )
            await self._db.commit()
            return cursor.rowcount

    async def purge_retention(self) -> None:
        assert self._db is not None
        cutoff = (datetime.utcnow() - timedelta(days=self._config.retention_days)).isoformat()
        async with self._lock:
            await self._db.execute("DELETE FROM telemetry WHERE ts < ?", (cutoff,))
            await self._db.execute("DELETE FROM ack WHERE ts < ?", (cutoff,))
            await self._db.execute("DELETE FROM events WHERE ts < ?", (cutoff,))
            await self._db.execute(
                "DELETE FROM queue_out WHERE ts < ? AND acked = 1",
                (cutoff,),
            )
            await self._db.commit()
        LOGGER.debug("Retention purge applied", extra={"cutoff": cutoff})

    async def store_telemetry(
        self,
        ts: datetime,
        asset_id: str,
        metrics: dict[str, float | int | None],
        rssi_dbm: int | None,
    ) -> None:
        assert self._db is not None
        rows = [
            (ts.isoformat(), asset_id, metric, value, "good", rssi_dbm)
            for metric, value in metrics.items()
            if value is not None
        ]
        if not rows:
            return
        async with self._lock:
            await self._db.executemany(
                """
                INSERT INTO telemetry (ts, asset_id, metric, value, quality, rssi_dbm)
                VALUES (?, ?, ?, ?, ?, ?)
                """,
                rows,
            )
            await self._db.commit()

    async def store_ack(
        self, ts: datetime, asset_id: str, correlation_id: str, ok: bool, message: str | None
    ) -> None:
        assert self._db is not None
        async with self._lock:
            await self._db.execute(
                """
                INSERT INTO ack (ts, asset_id, correlation_id, ok, message)
                VALUES (?, ?, ?, ?, ?)
                """,
                (ts.isoformat(), asset_id, correlation_id, int(ok), message),
            )
            await self._db.commit()

    async def store_event(
        self, ts: datetime, asset_id: str, event_type: str, payload_json: str
    ) -> None:
        assert self._db is not None
        async with self._lock:
            await self._db.execute(
                """
                INSERT INTO events (ts, asset_id, type, payload_json)
                VALUES (?, ?, ?, ?)
                """,
                (ts.isoformat(), asset_id, event_type, payload_json),
            )
            await self._db.commit()

    async def latest_telemetry(self, limit: int = 100) -> list[dict[str, object]]:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                """
                SELECT ts, asset_id, metric, value, quality, rssi_dbm
                FROM telemetry
                ORDER BY datetime(ts) DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        merged: dict[str, dict[str, object]] = {}
        for ts, asset_id, metric, value, quality, rssi in rows:
            asset = merged.setdefault(
                asset_id,
                {
                    "asset_id": asset_id,
                    "ts": ts,
                    "metrics": {},
                    "quality": {},
                    "rssi_dbm": rssi,
                },
            )
            asset["metrics"][metric] = value
            asset["quality"][metric] = quality
            if asset.get("ts") < ts:
                asset["ts"] = ts
            if asset.get("rssi_dbm") is None and rssi is not None:
                asset["rssi_dbm"] = rssi
        return list(merged.values())

    async def recent_acks(self, limit: int = 50) -> list[dict[str, object]]:
        assert self._db is not None
        async with self._lock:
            cursor = await self._db.execute(
                """
                SELECT ts, asset_id, correlation_id, ok, message
                FROM ack
                ORDER BY datetime(ts) DESC
                LIMIT ?
                """,
                (limit,),
            )
            rows = await cursor.fetchall()
        return [
            {
                "ts": ts,
                "asset_id": asset_id,
                "correlation_id": correlation_id,
                "ok": bool(ok),
                "message": message,
            }
            for ts, asset_id, correlation_id, ok, message in rows
        ]
