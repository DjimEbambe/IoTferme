from __future__ import annotations

import asyncio
from datetime import datetime, timezone

import pytest

from commands import CommandManager, CommandTimeout  # type: ignore


class FakeSerial:
    def __init__(self) -> None:
        self.sent: list[dict] = []

    async def send(self, payload: dict) -> None:
        self.sent.append(payload)


@pytest.mark.asyncio
async def test_command_manager_ack(storage):
    serial = FakeSerial()
    manager = CommandManager(serial, storage, timeout_seconds=0.5, max_retries=0, retry_backoff=0)
    payload = {
        "asset_id": "A-PP-01",
        "relay": {"lamp": "ON"},
        "correlation_id": "test-ack",
    }
    task = asyncio.create_task(manager.send(payload))
    await asyncio.sleep(0)  # allow send to queue
    await manager.handle_ack(
        {
            "asset_id": "A-PP-01",
            "correlation_id": "test-ack",
            "ok": True,
            "message": "applied",
            "ts": datetime.now(timezone.utc).isoformat(),
        }
    )
    result = await task
    assert result["ok"] is True


@pytest.mark.asyncio
async def test_command_manager_timeout(storage):
    serial = FakeSerial()
    manager = CommandManager(serial, storage, timeout_seconds=0.1, max_retries=1, retry_backoff=0.05)
    payload = {
        "asset_id": "A-PP-01",
        "relay": {"lamp": "ON"},
        "correlation_id": "test-timeout",
    }
    with pytest.raises(CommandTimeout):
        await manager.send(payload)
