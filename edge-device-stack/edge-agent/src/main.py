"""Edge agent FastAPI entrypoint."""
from __future__ import annotations

import asyncio
import json
import logging
import secrets
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from apscheduler.schedulers.asyncio import AsyncIOScheduler
from fastapi import Depends, FastAPI, HTTPException, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.security import HTTPBasic
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates

from .backlog import BacklogManager
from .commands import CommandManager, CommandTimeout
from .espnow_router import ESPNOWRouter
from .health import HealthMonitor
from .mqtt_client import MQTTClient, MQTTConfig
from .schemas import CommandPayload, TelemetryPayload
from .serial_bridge import SerialBridge, SerialConfig
from .settings import Settings, get_settings
from .storage import Storage, StorageConfig
from .utils.logging import configure_logging
from .utils.time_sync import build_sync_message

LOGGER = logging.getLogger(__name__)

BASE_DIR = Path(__file__).resolve().parents[1]
TEMPLATES_DIR = BASE_DIR / "templates"
STATIC_DIR = BASE_DIR / "static"

app = FastAPI(title="Edge Agent", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")
templates = Jinja2Templates(directory=str(TEMPLATES_DIR))
security = HTTPBasic()


@dataclass
class EdgeContext:
    settings: Settings
    storage: Storage
    backlog: BacklogManager
    mqtt: MQTTClient
    serial: SerialBridge
    commands: CommandManager
    health: HealthMonitor
    scheduler: AsyncIOScheduler
    router: ESPNOWRouter


def get_context() -> EdgeContext:
    ctx: EdgeContext | None = getattr(app.state, "context", None)
    if ctx is None:
        raise RuntimeError("Edge context not initialised")
    return ctx


def require_ui_auth(request: Request) -> None:
    settings = get_settings()
    if not settings.edge_bind_lan:
        return
    credentials = security(request)
    user_ok = secrets.compare_digest(credentials.username, settings.edge_basic_auth_user)
    pass_ok = secrets.compare_digest(
        credentials.password,
        settings.edge_basic_auth_pass.get_secret_value(),
    )
    if not (user_ok and pass_ok):
        raise HTTPException(status_code=401, detail="Unauthorized", headers={"WWW-Authenticate": "Basic"})


async def publish_with_backlog(ctx: EdgeContext, topic: str, payload: dict[str, Any], qos: int) -> None:
    try:
        await asyncio.wait_for(ctx.mqtt.publish(topic, payload, qos=qos), timeout=2)
    except (asyncio.TimeoutError, Exception) as exc:  # noqa: BLE001
        LOGGER.warning("Publish fallback to backlog", extra={"topic": topic, "err": str(exc)})
        await ctx.backlog.enqueue(topic, payload, qos, payload.get("idempotency_key"))


async def handle_serial(ctx: EdgeContext, payload: dict[str, Any]) -> None:
    msg_type = payload.get("type")
    if msg_type == "telemetry":
        await handle_telemetry(ctx, payload)
    elif msg_type == "ack":
        await ctx.commands.handle_ack(payload)
        await publish_with_backlog(ctx, ctx.settings.ack_topic, payload, ctx.settings.mqtt_qos)
    elif msg_type == "status":
        detail = {k: v for k, v in payload.items() if k != "type"}
        await ctx.health.set_state("gateway", payload.get("status", "unknown"), detail)
        await publish_with_backlog(ctx, ctx.settings.status_topic, payload, ctx.settings.mqtt_qos)
    elif msg_type == "event":
        await ctx.storage.store_event(
            ts=datetime.utcnow(),
            asset_id=payload.get("asset_id", "unknown"),
            event_type=payload.get("event", "generic"),
            payload_json=json.dumps(payload),
        )
        await publish_with_backlog(ctx, f"{ctx.settings.base_topic}/status", payload, ctx.settings.mqtt_qos)
    else:
        LOGGER.debug("Unknown serial message", extra={"payload": payload})


async def handle_telemetry(ctx: EdgeContext, payload: dict[str, Any]) -> None:
    telemetry = TelemetryPayload.model_validate(payload)
    metrics = telemetry.metrics.model_dump(exclude_none=True)
    await ctx.storage.store_telemetry(
        telemetry.ts,
        telemetry.asset_id,
        metrics,
        telemetry.rssi_dbm,
    )
    mac = payload.get("mac")
    if mac:
        await ctx.router.register(mac, telemetry.asset_id, telemetry.fw)
        await ctx.router.touch(mac, telemetry.rssi_dbm, telemetry.fw)
    channel = payload.get("channel", "env")
    topic = ctx.settings.telemetry_topics.get(channel, ctx.settings.telemetry_topics["env"])
    await publish_with_backlog(ctx, topic, payload, ctx.settings.mqtt_qos)


async def handle_mqtt(ctx: EdgeContext, topic: str, payload: dict[str, Any]) -> None:
    try:
        command = CommandPayload.model_validate(payload)
    except Exception as exc:  # noqa: BLE001
        LOGGER.warning("Invalid command payload", extra={"topic": topic, "err": str(exc)})
        return
    try:
        response = await ctx.commands.send(command.model_dump(mode="json"))
        await publish_with_backlog(ctx, ctx.settings.ack_topic, response, ctx.settings.mqtt_qos)
    except CommandTimeout:
        LOGGER.error("Command timeout", extra={"corr": command.correlation_id})
        await publish_with_backlog(
            ctx,
            ctx.settings.ack_topic,
            {
                "asset_id": command.asset_id,
                "correlation_id": command.correlation_id,
                "ok": False,
                "message": "timeout",
                "ts": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            },
            ctx.settings.mqtt_qos,
        )


async def mqtt_handler(topic: str, payload: dict[str, Any]) -> None:
    ctx = get_context()
    await handle_mqtt(ctx, topic, payload)


async def serial_handler(payload: dict[str, Any]) -> None:
    ctx = get_context()
    await handle_serial(ctx, payload)


async def update_link_health() -> None:
    ctx = get_context()
    await ctx.health.set_state(
        "mqtt",
        "up" if ctx.mqtt.is_connected() else "down",
        {"connected": ctx.mqtt.is_connected()},
    )
    await ctx.health.set_state(
        "serial",
        "up" if ctx.serial.is_connected() else "down",
        {"connected": ctx.serial.is_connected()},
    )
    backlog = await ctx.backlog.stats()
    await ctx.health.set_state(
        "backlog",
        "ok" if backlog.get("queued", 0) < 1000 else "degraded",
        backlog,
    )


async def send_time_sync() -> None:
    ctx = get_context()
    payload = build_sync_message(offset_ms=0)
    await ctx.serial.send(payload)


@app.on_event("startup")
async def on_startup() -> None:  # noqa: D401
    settings = get_settings()
    configure_logging(settings.log_level, str(settings.log_dir), settings.log_json)
    storage = Storage(StorageConfig(settings.sqlite_path, settings.retention_days))
    await storage.connect()
    health = HealthMonitor()
    router = ESPNOWRouter()
    try:
        serial_codec = getattr(settings, "serial_codec")
    except AttributeError:
        LOGGER.warning("serial_codec missing from settings, defaulting to msgpack")
        serial_codec = "msgpack"
    if serial_codec not in {"cbor", "msgpack"}:
        LOGGER.warning("Unsupported serial codec %s, defaulting to msgpack", serial_codec)
        serial_codec = "msgpack"

    serial = SerialBridge(
        SerialConfig(
            device=settings.usb_device,
            baudrate=settings.serial_baud,
            retry_seconds=settings.serial_retry_seconds,
            codec=serial_codec,
        ),
        serial_handler,
    )
    mqtt = MQTTClient(
        MQTTConfig(
            uri=str(settings.mqtt_uri),
            username=settings.mqtt_username,
            password=settings.mqtt_password.get_secret_value(),
            keepalive=settings.mqtt_keepalive,
            use_tls=settings.mqtt_use_tls,
            ca_file=str(settings.mqtt_ca_file) if settings.mqtt_ca_file else None,
            cert_file=str(settings.mqtt_cert_file) if settings.mqtt_cert_file else None,
            key_file=str(settings.mqtt_key_file) if settings.mqtt_key_file else None,
            lwt_topic=settings.mqtt_lwt_topic or settings.status_topic,
            lwt_payload=settings.mqtt_lwt_payload
            or json.dumps({"status": "offline", "ts": datetime.utcnow().isoformat()}),
            qos=settings.mqtt_qos,
        ),
        mqtt_handler,
    )
    backlog = BacklogManager(storage, mqtt.publish, settings.backlog_max_batch, settings.backlog_max_rate)
    commands = CommandManager(
        serial,
        storage,
        settings.cmd_timeout_seconds,
        settings.cmd_max_retries,
        settings.cmd_retry_backoff_seconds,
    )
    scheduler = AsyncIOScheduler()

    app.state.context = EdgeContext(
        settings=settings,
        storage=storage,
        backlog=backlog,
        mqtt=mqtt,
        serial=serial,
        commands=commands,
        health=health,
        scheduler=scheduler,
        router=router,
    )

    scheduler.add_job(storage.purge_retention, "cron", hour=3, minute=0)
    scheduler.add_job(send_time_sync, "interval", hours=settings.time_sync_interval_hours)
    scheduler.add_job(update_link_health, "interval", seconds=15)
    scheduler.start()

    mqtt.subscribe(f"v1/farm/{settings.site}/+/cmd", qos=settings.mqtt_qos)
    backlog.start()
    await mqtt.start()
    await serial.start()
    await publish_with_backlog(
        get_context(),
        settings.status_topic,
        {
            "status": "online",
            "ts": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            "site": settings.site,
            "device": settings.device_id,
        },
        qos=settings.mqtt_qos,
    )
    await update_link_health()


@app.on_event("shutdown")
async def on_shutdown() -> None:  # noqa: D401
    ctx: EdgeContext | None = getattr(app.state, "context", None)
    if ctx is None:
        return
    await ctx.serial.stop()
    await ctx.mqtt.stop()
    await ctx.backlog.stop()
    await ctx.storage.close()
    ctx.scheduler.shutdown(wait=False)


@app.get("/health")
async def get_health(context: EdgeContext = Depends(get_context)) -> JSONResponse:
    snapshot = await context.health.snapshot()
    return JSONResponse({"status": "ok", "components": snapshot})


@app.get("/", response_class=HTMLResponse)
async def ui_status(request: Request, _=Depends(require_ui_auth)) -> HTMLResponse:
    ctx = get_context()
    stats = await ctx.backlog.stats()
    telemetry = await ctx.storage.latest_telemetry()
    devices = await ctx.router.device_snapshot()
    health = await ctx.health.snapshot()
    return templates.TemplateResponse(
        "status.html",
        {
            "request": request,
            "site": ctx.settings.site,
            "device": ctx.settings.device_id,
            "backlog": stats,
            "telemetry": telemetry,
            "devices": devices,
            "health": health,
        },
    )


@app.get("/buffer", response_class=HTMLResponse)
async def ui_buffer(request: Request, _=Depends(require_ui_auth)) -> HTMLResponse:
    ctx = get_context()
    stats = await ctx.backlog.stats()
    head = await ctx.storage.backlog_entries(limit=100)
    return templates.TemplateResponse(
        "buffer.html",
        {
            "request": request,
            "site": ctx.settings.site,
            "device": ctx.settings.device_id,
            "stats": stats,
            "entries": head,
        },
    )


@app.get("/diag", response_class=HTMLResponse)
async def ui_diag(request: Request, _=Depends(require_ui_auth)) -> HTMLResponse:
    ctx = get_context()
    jobs = [
        {
            "id": job.id,
            "next_run_time": job.next_run_time.isoformat() if job.next_run_time else None,
            "trigger": str(job.trigger),
        }
        for job in ctx.scheduler.get_jobs()
    ]
    devices = await ctx.router.device_snapshot()
    return templates.TemplateResponse(
        "diag.html",
        {
            "request": request,
            "site": ctx.settings.site,
            "device": ctx.settings.device_id,
            "jobs": jobs,
            "devices": devices,
        },
    )


@app.get("/commands", response_class=HTMLResponse)
async def ui_commands(request: Request, _=Depends(require_ui_auth)) -> HTMLResponse:
    ctx = get_context()
    devices = await ctx.router.device_snapshot()
    return templates.TemplateResponse(
        "commands.html",
        {
            "request": request,
            "site": ctx.settings.site,
            "device": ctx.settings.device_id,
            "devices": devices,
        },
    )


from .api import router as api_router  # noqa: E402  pylint: disable=wrong-import-position

app.include_router(api_router)
