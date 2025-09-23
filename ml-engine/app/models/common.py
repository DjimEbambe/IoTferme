from datetime import datetime
from typing import List
from pydantic import BaseModel, Field


class SeriesPoint(BaseModel):
    ts: datetime = Field(..., description="Timestamp UTC")
    value: float = Field(..., description="Measured value")


class SeriesRequest(BaseModel):
    series: List[SeriesPoint]

    def values(self):
        return [point.value for point in self.series]
