from fastapi import APIRouter, Depends, HTTPException, status
from ..deps import verify_api_key
from ...models.predict import PredictRequest, PredictResponse
from ...services.predict import forecast

router = APIRouter(prefix='/ml', tags=['ml'], dependencies=[Depends(verify_api_key)])


@router.post('/predict', response_model=PredictResponse)
async def predict(payload: PredictRequest):
    try:
        forecast_points = forecast(payload)
    except ValueError as exc:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(exc)) from exc
    return PredictResponse(forecast=forecast_points)
