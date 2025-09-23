from fastapi import APIRouter, Depends
from ..deps import verify_api_key
from ...models.schedule import ScheduleRequest, ScheduleResponse
from ...services.schedule import build_schedule

router = APIRouter(prefix='/ml', tags=['ml'], dependencies=[Depends(verify_api_key)])


@router.post('/schedule', response_model=ScheduleResponse)
async def schedule(payload: ScheduleRequest):
    return build_schedule(payload)
