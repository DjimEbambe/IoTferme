"""API routers exposed by the FastAPI application."""
from __future__ import annotations

from fastapi import APIRouter

from . import routes_buffer, routes_cmd, routes_diag, routes_status

router = APIRouter()
router.include_router(routes_status.router)
router.include_router(routes_cmd.router)
router.include_router(routes_buffer.router)
router.include_router(routes_diag.router)

__all__ = ["router"]
