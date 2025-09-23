"""Buffer and backlog management endpoints."""
from __future__ import annotations

from fastapi import APIRouter, Depends

from ..main import get_context

router = APIRouter(prefix="/api/buffer", tags=["buffer"])


@router.get("")
async def buffer_status(context=Depends(get_context)) -> dict:
    stats = await context.backlog.stats()
    entries = await context.storage.backlog_entries(limit=50)
    return {
        "stats": stats,
        "head": entries,
    }


@router.post("/purge")
async def purge_buffer(context=Depends(get_context)) -> dict:
    deleted = await context.storage.purge_backlog()
    return {"purged": deleted}


@router.post("/replay")
async def replay(context=Depends(get_context)) -> dict:
    context.backlog.start()
    return {"status": "queued"}
