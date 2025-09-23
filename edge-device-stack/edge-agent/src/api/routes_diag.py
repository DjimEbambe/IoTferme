"""Diagnostics endpoints for field troubleshooting."""
from __future__ import annotations

import asyncio
from datetime import datetime, timezone

from fastapi import APIRouter, Depends
from pydantic import BaseModel, Field

from ..main import get_context
from ..utils.time_sync import build_sync_message

router = APIRouter(prefix="/api/diag", tags=["diag"])


class PingRequest(BaseModel):
    asset_id: str
    mac: str | None = None
    correlation_id: str | None = None


class PairingWindowRequest(BaseModel):
    duration_s: int = Field(default=120, ge=30, le=600)


class SetMacRequest(BaseModel):
    mac: str = Field(pattern=r"(?i)^[0-9a-f]{2}(:[0-9a-f]{2}){5}$")
    persist: bool = False


@router.post("/ping")
async def ping_device(request: PingRequest, context=Depends(get_context)) -> dict:
    correlation_id = request.correlation_id or f"ping-{int(datetime.now(tz=timezone.utc).timestamp())}"
    payload = {
        "type": "ping",
        "asset_id": request.asset_id,
        "correlation_id": correlation_id,
    }
    if request.mac:
        payload["mac"] = request.mac
    await context.serial.send(payload)
    return {"sent": True, "correlation_id": correlation_id}


@router.post("/pair/begin")
async def pair_begin(request: PairingWindowRequest, context=Depends(get_context)) -> dict:
    await context.serial.send({"type": "pair_begin", "duration_s": request.duration_s})
    return {"status": "pairing", "duration_s": request.duration_s}


@router.post("/pair/end")
async def pair_end(context=Depends(get_context)) -> dict:
    await context.serial.send({"type": "pair_end"})
    return {"status": "pairing_closed"}


@router.post("/time-sync")
async def force_time_sync(context=Depends(get_context)) -> dict:
    payload = build_sync_message(offset_ms=0)
    await context.serial.send(payload)
    return {"status": "sent", "ts": payload["ts"]}


@router.post("/set-mac")
async def set_gateway_mac(request: SetMacRequest, context=Depends(get_context)) -> dict:
    payload = {
        "type": "cfg",
        "op": "set_mac",
        "mac": request.mac.lower(),
        "persist": request.persist,
    }
    await context.serial.send(payload)
    return {"status": "sent", "mac": request.mac.lower(), "persist": request.persist}


@router.get("/tasks")
async def scheduler_jobs(context=Depends(get_context)) -> dict:
    jobs = [
        {
            "id": job.id,
            "next_run_time": job.next_run_time.isoformat() if job.next_run_time else None,
            "trigger": str(job.trigger),
        }
        for job in context.scheduler.get_jobs()
    ]
    return {"jobs": jobs}


@router.post("/reset-backlog")
async def reset_backlog(context=Depends(get_context)) -> dict:
    await context.backlog.stop()
    await asyncio.sleep(0)
    await context.backlog.enqueue(
        context.settings.status_topic,
        {
            "ts": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            "status": "manual-reset",
            "site": context.settings.site,
            "device": context.settings.device_id,
        },
        qos=context.settings.mqtt_qos,
        idempotency_key=None,
    )
    context.backlog.start()
    return {"status": "reset_sent"}
