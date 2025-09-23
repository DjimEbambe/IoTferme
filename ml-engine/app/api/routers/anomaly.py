from fastapi import APIRouter, Depends
from ..deps import verify_api_key
from ...models.anomaly import AnomalyRequest, AnomalyResponse
from ...services.anomaly import detect_anomalies

router = APIRouter(prefix='/ml', tags=['ml'], dependencies=[Depends(verify_api_key)])


@router.post('/anomaly', response_model=AnomalyResponse)
async def anomaly_detection(payload: AnomalyRequest):
    anomalies = detect_anomalies(payload)
    return AnomalyResponse(anomalies=anomalies)
