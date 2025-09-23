"""Command dispatcher bridging REST/MQTT to ESP-NOW devices."""
from __future__ import annotations

import asyncio
import logging
import uuid
from datetime import datetime
from typing import Any, Dict

from .serial_bridge import SerialBridge
from .storage import Storage

LOGGER = logging.getLogger(__name__)


class CommandTimeout(Exception):
    pass


class CommandManager:
    def __init__(
        self,
        serial_bridge: SerialBridge,
        storage: Storage,
        timeout_seconds: int,
        max_retries: int,
        retry_backoff: int,
    ) -> None:
        self._serial = serial_bridge
        self._storage = storage
        self._timeout = timeout_seconds
        self._max_retries = max_retries
        self._retry_backoff = retry_backoff
        self._pending: dict[str, asyncio.Future] = {}
        self._lock = asyncio.Lock()

    async def send(self, command: dict[str, Any]) -> dict[str, Any]:
        correlation_id = command.get("correlation_id") or str(uuid.uuid4())
        command["correlation_id"] = correlation_id
        fut = asyncio.get_running_loop().create_future()
        async with self._lock:
            if correlation_id in self._pending:
                raise RuntimeError("Duplicate correlation id")
            self._pending[correlation_id] = fut
        attempt = 0
        while True:
            attempt += 1
            LOGGER.info(
                "Dispatch command", extra={"asset_id": command.get("asset_id"), "corr": correlation_id, "attempt": attempt}
            )
            await self._serial.send({"type": "cmd", **command})
            try:
                result = await asyncio.wait_for(fut, timeout=self._timeout)
                return result
            except asyncio.TimeoutError as exc:
                if attempt > self._max_retries:
                    fut.cancel()
                    async with self._lock:
                        self._pending.pop(correlation_id, None)
                    raise CommandTimeout("Command timed out") from exc
                await asyncio.sleep(self._retry_backoff)

    async def handle_ack(self, payload: dict[str, Any]) -> None:
        correlation_id = payload.get("correlation_id")
        if not correlation_id:
            LOGGER.warning("ACK without correlation_id", extra={"payload": payload})
            return
        async with self._lock:
            fut = self._pending.pop(correlation_id, None)
        if fut is None:
            LOGGER.warning("Received stray ACK", extra={"corr": correlation_id})
            return
        if not fut.done():
            fut.set_result(payload)
        ts = datetime.utcnow()
        await self._storage.store_ack(
            ts=ts,
            asset_id=payload.get("asset_id", "unknown"),
            correlation_id=correlation_id,
            ok=payload.get("ok", True),
            message=payload.get("message"),
        )

    async def pending_count(self) -> int:
        async with self._lock:
            return len(self._pending)
