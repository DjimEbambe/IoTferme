"""Status and metrics endpoints for the local UI."""
from __future__ import annotations

from fastapi import APIRouter, Depends

from ..main import get_context

router = APIRouter(prefix="/api", tags=["status"])


@router.get("/status")
async def get_status(context=Depends(get_context)) -> dict:
    backlog = await context.backlog.stats()
    health = await context.health.snapshot()
    pending_cmd = await context.commands.pending_count()
    return {
        "site": context.settings.site,
        "gateway": context.settings.device_id,
        "mqtt_connected": context.mqtt.is_connected(),
        "serial_connected": context.serial.is_connected(),
        "backlog": backlog,
        "pending_commands": pending_cmd,
        "health": health,
    }


@router.get("/metrics")
async def get_metrics(context=Depends(get_context)) -> dict:
    telemetry = await context.storage.latest_telemetry()
    acks = await context.storage.recent_acks()
    return {
        "telemetry": telemetry,
        "acks": acks,
    }


@router.get("/devices")
async def get_devices(context=Depends(get_context)) -> dict:
    devices = await context.router.device_snapshot()
    return {"devices": devices}
