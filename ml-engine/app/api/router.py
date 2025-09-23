from fastapi import APIRouter
from .routers.anomaly import router as anomaly_router
from .routers.predict import router as predict_router
from .routers.optimize import router as optimize_router
from .routers.schedule import router as schedule_router

api_router = APIRouter()
api_router.include_router(anomaly_router)
api_router.include_router(predict_router)
api_router.include_router(optimize_router)
api_router.include_router(schedule_router)

__all__ = ['api_router']
