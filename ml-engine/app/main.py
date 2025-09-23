import asyncio
import logging
import os
from uuid import uuid4

from fastapi import FastAPI, Depends, HTTPException, Request
from fastapi.responses import JSONResponse

from .api.router import api_router
from .api.deps import verify_api_key
from .jobs.scheduler import configure_scheduler
from .utils.settings import get_settings

settings = get_settings()
logging.basicConfig(level=logging.INFO, format='%(asctime)s %(name)s %(levelname)s %(message)s')
logger = logging.getLogger('farmstack-ml')
os.environ['TZ'] = settings.tz

app = FastAPI(
    title='FarmStack ML Engine',
    version='0.1.0',
    description='Services ML pour la ferme avicole (anomalies, prévisions, optimisation de ration).',
)
app.include_router(api_router)


@app.middleware('http')
async def request_context(request: Request, call_next):
    request_id = request.headers.get('X-Request-ID', str(uuid4()))
    request.state.request_id = request_id
    response = await call_next(request)
    response.headers['X-Request-ID'] = request_id
    return response


@app.exception_handler(HTTPException)
async def http_exception_handler(request: Request, exc: HTTPException):
    request_id = getattr(request.state, 'request_id', None)
    detail = exc.detail if isinstance(exc.detail, str) else exc.detail
    code = 'REQUEST_ERROR'
    if isinstance(exc.detail, dict):
        code = exc.detail.get('code', code)
        detail = exc.detail.get('message', 'Request error')
    else:
        detail = detail or 'Request error'
    logger.warning('HTTP exception', extra={'request_id': request_id, 'status': exc.status_code})
    return JSONResponse(
        {
            'error': detail,
            'code': code,
            'request_id': request_id,
        },
        status_code=exc.status_code,
    )


@app.exception_handler(Exception)
async def unhandled_exception_handler(request: Request, exc: Exception):
    request_id = getattr(request.state, 'request_id', None)
    logger.error('Unhandled ML engine error', exc_info=exc, extra={'request_id': request_id})
    return JSONResponse(
        {
            'error': 'Unhandled server error',
            'code': 'SERVER_ERROR',
            'request_id': request_id,
        },
        status_code=500,
    )


@app.on_event('startup')
async def on_startup():
    os.makedirs(settings.model_store_path, exist_ok=True)
    os.makedirs(settings.report_output_path, exist_ok=True)
    configure_scheduler()


@app.on_event('shutdown')
async def on_shutdown():
    await asyncio.sleep(0)  # placeholder for graceful shutdown


@app.get('/ml/health')
async def health(_: bool = Depends(verify_api_key)):
    return JSONResponse(
        {
            'status': 'ok',
            'env': settings.env,
            'model_store': settings.model_store_path,
        }
    )
