"""Routing abstraction for ESP-NOW devices behind the gateway."""
from __future__ import annotations

import asyncio
import logging
from dataclasses import dataclass, field
from datetime import datetime
from typing import Dict, Optional

LOGGER = logging.getLogger(__name__)


@dataclass
class DeviceInfo:
    asset_id: str
    mac: str
    rssi_dbm: Optional[int] = None
    last_seen: datetime = field(default_factory=datetime.utcnow)
    fw: Optional[str] = None


class ESPNOWRouter:
    def __init__(self) -> None:
        self._devices: Dict[str, DeviceInfo] = {}
        self._lock = asyncio.Lock()

    async def register(self, mac: str, asset_id: str, fw: Optional[str] = None) -> None:
        async with self._lock:
            existing = self._devices.get(mac)
            if existing and existing.asset_id == asset_id:
                if fw:
                    existing.fw = fw
                LOGGER.debug("Device already registered", extra={"mac": mac, "asset_id": asset_id})
                return
            self._devices[mac] = DeviceInfo(asset_id=asset_id, mac=mac, fw=fw)
            LOGGER.info("Device registered", extra={"mac": mac, "asset_id": asset_id})

    async def touch(self, mac: str, rssi_dbm: Optional[int], fw: Optional[str]) -> None:
        async with self._lock:
            if mac not in self._devices:
                LOGGER.warning("Unknown device MAC", extra={"mac": mac})
                return
            device = self._devices[mac]
            device.last_seen = datetime.utcnow()
            device.rssi_dbm = rssi_dbm
            if fw:
                device.fw = fw

    async def resolve_asset(self, mac: str) -> Optional[str]:
        async with self._lock:
            info = self._devices.get(mac)
            return info.asset_id if info else None

    async def device_snapshot(self) -> list[dict[str, str | int | None]]:
        async with self._lock:
            return [
                {
                    "mac": device.mac,
                    "asset_id": device.asset_id,
                    "rssi_dbm": device.rssi_dbm,
                    "last_seen": device.last_seen.isoformat(),
                    "fw": device.fw,
                }
                for device in self._devices.values()
            ]

    async def devices_by_asset(self) -> Dict[str, DeviceInfo]:
        async with self._lock:
            return dict(self._devices)
