from __future__ import annotations

import asyncio
from datetime import datetime, timezone

import pytest

from backlog import BacklogManager  # type: ignore


class DummyInfo:
    def wait_for_publish(self) -> None:  # noqa: D401
        return None


class PublishStub:
    def __init__(self) -> None:
        self.published: list[tuple[str, dict, int]] = []

    async def __call__(self, topic: str, payload: dict, qos: int):
        self.published.append((topic, payload, qos))
        return DummyInfo()


@pytest.mark.asyncio
async def test_backlog_flush(storage):
    publisher = PublishStub()
    manager = BacklogManager(storage, publisher, batch_size=10, max_rate=100)
    manager.start()
    for i in range(3):
        await manager.enqueue(
            topic=f"topic/{i}",
            payload={"id": i, "ts": datetime.now(timezone.utc).isoformat()},
            qos=1,
            idempotency_key=f"key-{i}",
        )
    await asyncio.sleep(0.2)
    await manager.stop()
    stats = await storage.backlog_counts()
    assert stats["queued"] == 0
    assert len(publisher.published) == 3
