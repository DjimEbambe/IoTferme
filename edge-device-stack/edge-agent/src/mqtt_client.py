"""MQTT client wrapper using paho-mqtt."""
from __future__ import annotations

import asyncio
import json
import logging
from dataclasses import dataclass
from typing import Awaitable, Callable
from urllib.parse import urlparse

import paho.mqtt.client as mqtt

LOGGER = logging.getLogger(__name__)

MessageHandler = Callable[[str, dict], Awaitable[None]]


@dataclass(slots=True)
class MQTTConfig:
    uri: str
    username: str
    password: str
    keepalive: int = 30
    use_tls: bool = True
    ca_file: str | None = None
    cert_file: str | None = None
    key_file: str | None = None
    lwt_topic: str | None = None
    lwt_payload: str | None = None
    qos: int = 1


class MQTTClient:
    def __init__(self, config: MQTTConfig, message_handler: MessageHandler) -> None:
        self._config = config
        self._handler = message_handler
        parsed = urlparse(config.uri)
        self._host = parsed.hostname or "localhost"
        self._port = parsed.port or (8883 if parsed.scheme.endswith("s") else 1883)
        client_id = f"edge-agent-{self._host}"[:23]
        self._client = mqtt.Client(client_id=client_id, clean_session=False)
        self._client.username_pw_set(config.username, config.password)
        if config.use_tls:
            self._client.tls_set(
                ca_certs=config.ca_file,
                certfile=config.cert_file,
                keyfile=config.key_file,
            )
        if config.lwt_topic and config.lwt_payload:
            self._client.will_set(config.lwt_topic, payload=config.lwt_payload, qos=config.qos, retain=True)
        self._client.on_connect = self._handle_connect
        self._client.on_message = self._handle_message
        self._client.on_disconnect = self._handle_disconnect
        self._connected = asyncio.Event()
        self._subscriptions: list[tuple[str, int]] = []
        self._loop_task: asyncio.Task | None = None
        self._stop = asyncio.Event()
        self._publish_lock = asyncio.Lock()

    async def start(self) -> None:
        self._stop.clear()
        self._loop_task = asyncio.create_task(self._loop())

    async def stop(self) -> None:
        self._stop.set()
        if self._loop_task:
            await self._loop_task
        self._client.loop_stop()
        self._client.disconnect()

    def subscribe(self, topic: str, qos: int = 1) -> None:
        self._subscriptions.append((topic, qos))
        if self._client.is_connected():
            self._client.subscribe(topic, qos=qos)

    async def publish(self, topic: str, payload: dict, qos: int = 1, retain: bool = False):
        data = json.dumps(payload, separators=(",", ":"))
        async with self._publish_lock:
            await self._connected.wait()
            info = self._client.publish(topic, payload=data, qos=qos, retain=retain)
            return info

    async def _loop(self) -> None:
        while not self._stop.is_set():
            try:
                self._client.connect(self._host, self._port, keepalive=self._config.keepalive)
                self._client.loop_start()
                await self._connected.wait()
                await self._stop.wait()
            except Exception as exc:  # noqa: BLE001
                LOGGER.error("MQTT connection failed", extra={"err": str(exc)})
                await asyncio.sleep(5)
            finally:
                self._client.loop_stop()
                self._connected.clear()
                if not self._stop.is_set():
                    await asyncio.sleep(5)

    def _handle_connect(self, client: mqtt.Client, userdata, flags, rc):  # noqa: ANN001, D401
        if rc == mqtt.MQTT_ERR_SUCCESS:
            LOGGER.info("MQTT connected", extra={"rc": rc})
            for topic, qos in self._subscriptions:
                client.subscribe(topic, qos=qos)
            self._connected.set()
        else:
            LOGGER.error("MQTT connect error", extra={"rc": rc})

    def _handle_disconnect(self, client: mqtt.Client, userdata, rc):  # noqa: ANN001, D401
        LOGGER.warning("MQTT disconnected", extra={"rc": rc})
        self._connected.clear()

    def is_connected(self) -> bool:
        return self._client.is_connected() and self._connected.is_set()

    def _handle_message(self, client: mqtt.Client, userdata, msg: mqtt.MQTTMessage):  # noqa: ANN001, D401
        try:
            payload = json.loads(msg.payload.decode("utf-8")) if msg.payload else {}
        except json.JSONDecodeError:
            LOGGER.warning("Invalid JSON on topic", extra={"topic": msg.topic})
            return
        asyncio.create_task(self._handler(msg.topic, payload))
