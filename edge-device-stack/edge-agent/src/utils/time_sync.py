"""Time synchronisation helpers."""
from __future__ import annotations

from datetime import datetime, timezone

from .cbor_codec import isoformat


def build_sync_message(offset_ms: int) -> dict[str, str | int]:
    utc_now = datetime.now(tz=timezone.utc)
    return {
        "type": "time_sync",
        "ts": isoformat(utc_now),
        "offset_ms": offset_ms,
        "epoch_ms": int(utc_now.timestamp() * 1000),
    }


def compute_offset(target_ts: datetime) -> int:
    now = datetime.now(tz=timezone.utc)
    delta = target_ts - now
    return int(delta.total_seconds() * 1000)
