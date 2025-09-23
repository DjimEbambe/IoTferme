"""Backlog manager bridging SQLite queue with MQTT."""
from __future__ import annotations

import asyncio
import json
import logging
from datetime import datetime

from paho.mqtt.client import MQTTMessageInfo

from .storage import Storage

LOGGER = logging.getLogger(__name__)


class BacklogManager:
    def __init__(
        self,
        storage: Storage,
        publish_cb,
        batch_size: int,
        max_rate: int,
    ) -> None:
        self._storage = storage
        self._publish_cb = publish_cb
        self._batch_size = batch_size
        self._max_rate = max_rate
        self._stop = asyncio.Event()
        self._task: asyncio.Task | None = None
        self._adaptive_delay = max(1 / max_rate, 0.001)

    def start(self) -> None:
        if self._task is None or self._task.done():
            self._stop.clear()
            self._task = asyncio.create_task(self._loop())

    async def stop(self) -> None:
        self._stop.set()
        if self._task:
            await self._task

    async def enqueue(self, topic: str, payload: dict, qos: int, idempotency_key: str | None) -> int:
        ts = datetime.utcnow()
        row_id = await self._storage.put_backlog(ts, topic, json.dumps(payload), qos, idempotency_key)
        LOGGER.debug("Backlog enqueued", extra={"id": row_id, "topic": topic})
        return row_id

    async def _loop(self) -> None:
        while not self._stop.is_set():
            rows = await self._storage.fetch_backlog(self._batch_size)
            if not rows:
                await asyncio.sleep(1)
                continue
            success_ids: list[int] = []
            for row_id, topic, payload_json, qos in rows:
                try:
                    message = json.loads(payload_json)
                    info: MQTTMessageInfo = await self._publish_cb(topic, message, qos)
                    info.wait_for_publish()
                    success_ids.append(row_id)
                except Exception as exc:  # noqa: BLE001
                    LOGGER.warning("Backlog publish failed", extra={"id": row_id, "err": str(exc)})
                    break
            if success_ids:
                await self._storage.mark_sent(success_ids, acked=True)
            await asyncio.sleep(self._adaptive_delay)

    async def stats(self) -> dict[str, int]:
        metrics = await self._storage.backlog_counts()
        queued = metrics.get("queued", 0)
        if queued > 100_000:
            self._adaptive_delay = max(1 / max(self._max_rate // 5, 1), 0.01)
        elif queued > 10_000:
            self._adaptive_delay = max(1 / max(self._max_rate // 2, 1), 0.005)
        else:
            self._adaptive_delay = max(1 / self._max_rate, 0.001)
        return metrics
