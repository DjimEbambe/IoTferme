"""Edge health probes."""
from __future__ import annotations

import asyncio
import logging
from dataclasses import dataclass, field
from datetime import datetime
from typing import Dict, Optional

LOGGER = logging.getLogger(__name__)


@dataclass
class LinkState:
    status: str = "unknown"
    detail: dict[str, str | int | float] = field(default_factory=dict)
    updated_at: datetime = field(default_factory=datetime.utcnow)


class HealthMonitor:
    def __init__(self) -> None:
        self._states: Dict[str, LinkState] = {}
        self._lock = asyncio.Lock()

    async def set_state(self, key: str, status: str, detail: Optional[dict] = None) -> None:
        async with self._lock:
            self._states[key] = LinkState(status=status, detail=detail or {})
            LOGGER.debug("Health state updated", extra={"key": key, "status": status})

    async def snapshot(self) -> dict[str, dict]:
        async with self._lock:
            return {
                key: {
                    "status": state.status,
                    "detail": state.detail,
                    "updated_at": state.updated_at.isoformat(),
                }
                for key, state in self._states.items()
            }
