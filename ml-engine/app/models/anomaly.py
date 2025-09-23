from typing import Literal, List
from pydantic import BaseModel, Field
from .common import SeriesPoint, SeriesRequest


class AnomalyRequest(SeriesRequest):
    method: Literal['zscore', 'iqr', 'changepoint'] = Field('zscore', description='Méthode de détection')
    sensitivity: float = Field(3.0, ge=0.5, le=10.0)


class AnomalyResult(BaseModel):
    index: int
    point: SeriesPoint
    score: float


class AnomalyResponse(BaseModel):
    anomalies: List[AnomalyResult]
