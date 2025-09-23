"""CBOR serialization helpers with fallback hints."""
from __future__ import annotations

from datetime import datetime
from typing import Any

import cbor2


class CBORCodec:
    def __init__(self, use_canonical: bool = True) -> None:
        self._canonical = use_canonical

    def dumps(self, payload: dict[str, Any]) -> bytes:
        return cbor2.dumps(payload, canonical=self._canonical, datetime_as_timestamp=True)

    def loads(self, buffer: bytes) -> dict[str, Any]:
        value = cbor2.loads(buffer)
        if isinstance(value, dict):
            return value
        if isinstance(value, list):
            return {"list": value}
        return {"value": value}


codec = CBORCodec()


def isoformat(ts: datetime) -> str:
    return ts.replace(microsecond=0).isoformat().replace("+00:00", "Z")
