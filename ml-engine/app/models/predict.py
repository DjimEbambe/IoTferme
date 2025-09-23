from typing import List
from pydantic import BaseModel, Field
from .common import SeriesPoint, SeriesRequest


class PredictRequest(SeriesRequest):
    horizon: int = Field(24, ge=1, le=168)
    interval_hours: int = Field(1, ge=1, le=24)


class PredictPoint(SeriesPoint):
    pass


class PredictResponse(BaseModel):
    forecast: List[PredictPoint]
