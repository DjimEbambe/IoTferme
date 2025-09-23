"""Command endpoints for local operations and diagnostics."""
from __future__ import annotations

from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel, Field

from ..main import get_context
from ..schemas import CommandPayload, CommandSequenceStep

router = APIRouter(prefix="/api/cmd", tags=["commands"])


class TestRelayRequest(BaseModel):
    asset_id: str
    channel: str = Field(description="Logical relay alias (lamp, fan, heater, door, pump)")
    state: str = Field(pattern="^(ON|OFF)$")
    duration_s: int | None = Field(default=None, ge=1, le=3600)


@router.post("", name="send_command")
async def post_command(payload: CommandPayload, context=Depends(get_context)) -> dict:
    try:
        result = await context.commands.send(payload.model_dump(mode="json"))
    except Exception as exc:  # noqa: BLE001
        raise HTTPException(status_code=504, detail=str(exc)) from exc
    return result


@router.get("/pending")
async def pending(context=Depends(get_context)) -> dict:
    count = await context.commands.pending_count()
    return {"pending": count}


@router.post("/test-relay")
async def test_relay(request: TestRelayRequest, context=Depends(get_context)) -> dict:
    correlation_id = f"ui-test-{int(datetime.now(tz=timezone.utc).timestamp())}"
    sequence: list[CommandSequenceStep] = []
    if request.duration_s:
        sequence = [CommandSequenceStep(act=request.channel, dur_s=request.duration_s)]
    command = CommandPayload(
        asset_id=request.asset_id,
        relay={request.channel: request.state},
        correlation_id=correlation_id,
        setpoints={},
        sequence=sequence,
    )
    result = await context.commands.send(command.model_dump(mode="json"))
    return result
