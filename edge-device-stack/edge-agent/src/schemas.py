"""Shared payload schemas aligned with the cloud contracts."""
from __future__ import annotations

from datetime import datetime
from typing import Any, Dict, List, Optional

from pydantic import BaseModel, Field, RootModel

ISO8601 = "2025-09-17T12:03:20Z"


class BasePayload(BaseModel):
    ts: datetime = Field(description="UTC timestamp")
    site: str = Field(default="KIN-GOLIATH")
    device: str = Field(description="Device identifier")


class TelemetryMetrics(BaseModel):
    t_c: Optional[float] = None
    rh: Optional[float] = None
    mq135_ppm: Optional[float] = None
    lux: Optional[float] = None
    voltage_v: Optional[float] = None
    current_a: Optional[float] = None
    power_w: Optional[float] = None
    energy_wh: Optional[float] = None
    flow_lpm: Optional[float] = None
    tank_level_pct: Optional[float] = None
    incubator_temp_c: Optional[float] = None
    incubator_rh: Optional[float] = None


class TelemetryPayload(BasePayload):
    asset_id: str
    metrics: TelemetryMetrics
    rssi_dbm: Optional[int] = None
    fw: Optional[str] = None
    idempotency_key: str


class CommandSequenceStep(BaseModel):
    act: str
    dur_s: Optional[int] = Field(default=None, ge=0)
    wait_s: Optional[int] = Field(default=None, ge=0)


class CommandPayload(BaseModel):
    asset_id: str
    relay: Dict[str, str] = Field(default_factory=dict)
    setpoints: Dict[str, Any] = Field(default_factory=dict)
    sequence: List[CommandSequenceStep] = Field(default_factory=list)
    correlation_id: str
    ts: datetime = Field(default_factory=datetime.utcnow)
    issued_by: str = Field(default="edge-agent")


class AckPayload(BaseModel):
    asset_id: str
    correlation_id: str
    ok: bool
    message: Optional[str] = None
    ts: datetime = Field(default_factory=datetime.utcnow)


class StatusPayload(BaseModel):
    status: str
    correlation_id: Optional[str] = None
    ts: datetime = Field(default_factory=datetime.utcnow)
    detail: Dict[str, Any] = Field(default_factory=dict)


class EventPayload(BaseModel):
    asset_id: str
    type: str
    payload: Dict[str, Any]
    ts: datetime = Field(default_factory=datetime.utcnow)


class BacklogItem(BaseModel):
    id: Optional[int] = None
    ts: datetime
    topic: str
    payload_json: str
    qos: int = 1
    sent: bool = False
    acked: bool = False
    idempotency_key: Optional[str] = None


class BacklogBatch(RootModel[List[BacklogItem]]):
    pass
