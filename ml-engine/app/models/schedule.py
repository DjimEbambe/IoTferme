from typing import List
from pydantic import BaseModel, Field


class TaskItem(BaseModel):
    ts: str
    action: str
    target: str
    notes: str | None = None


class ScheduleRequest(BaseModel):
    lot_id: str = Field(..., description='Lot concerné')
    current_age_days: int = Field(..., ge=0)
    horizon_hours: int = Field(48, ge=1, le=168)
    avg_weight_kg: float = Field(1.0, ge=0)


class ScheduleResponse(BaseModel):
    tasks: List[TaskItem]
