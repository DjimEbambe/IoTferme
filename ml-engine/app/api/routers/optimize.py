from fastapi import APIRouter, Depends, HTTPException, status
from ..deps import verify_api_key
from ...models.optimize import OptimizeRationRequest, OptimizeRationResponse
from ...services.optimize_ration import optimize_ration

router = APIRouter(prefix='/ml', tags=['ml'], dependencies=[Depends(verify_api_key)])


@router.post('/optimize_ration', response_model=OptimizeRationResponse)
async def optimize(payload: OptimizeRationRequest):
    try:
        return optimize_ration(payload)
    except ValueError as exc:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(exc)) from exc
