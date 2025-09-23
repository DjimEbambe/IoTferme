"""USB CDC bridge using COBS framed packets with configurable payload codec."""
from __future__ import annotations

import asyncio
import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Awaitable, Callable, Literal

import msgpack
import serial

from .utils import cbor_codec, cobs

LOGGER = logging.getLogger(__name__)

PayloadHandler = Callable[[dict], Awaitable[None]]
Codec = Literal["cbor", "msgpack"]


@dataclass(slots=True)
class SerialConfig:
    device: Path
    baudrate: int = 921600
    retry_seconds: int = 5
    codec: Codec = "msgpack"


class SerialBridge:
    def __init__(self, config: SerialConfig, handler: PayloadHandler) -> None:
        self._config = config
        self._handler = handler
        self._stop = asyncio.Event()
        self._reader_task: asyncio.Task | None = None
        self._serial: serial.Serial | None = None
        self._write_lock = asyncio.Lock()

    async def start(self) -> None:
        LOGGER.info(
            "Starting SerialBridge", extra={"port": str(self._config.device), "codec": self._config.codec}
        )
        self._stop.clear()
        self._reader_task = asyncio.create_task(self._read_loop())

    async def stop(self) -> None:
        self._stop.set()
        if self._reader_task:
            await self._reader_task
        if self._serial and self._serial.is_open:
            self._serial.close()
            LOGGER.info("Serial port closed")

    async def send(self, payload: dict) -> None:
        encoded = self._encode(payload)
        frame = encoded + crc16(encoded).to_bytes(2, "big")
        stuffed = cobs.encode(frame)
        async with self._write_lock:
            await asyncio.to_thread(self._write_raw, stuffed)

    def is_connected(self) -> bool:
        return bool(self._serial and self._serial.is_open)

    async def _ensure_open(self) -> serial.Serial:
        if self._serial and self._serial.is_open:
            return self._serial
        while not self._stop.is_set():
            try:
                self._serial = serial.Serial(
                    str(self._config.device),
                    baudrate=self._config.baudrate,
                    timeout=1,
                )
                LOGGER.info("Serial port opened", extra={"port": str(self._config.device)})
                return self._serial
            except serial.SerialException as exc:  # type: ignore[attr-defined]
                LOGGER.warning(
                    "Serial port unavailable, retrying",
                    extra={"port": str(self._config.device), "err": str(exc)},
                )
                await asyncio.sleep(self._config.retry_seconds)
        raise RuntimeError("SerialBridge stopped")

    def _write_raw(self, data: bytes) -> None:
        port = self._serial
        if not port or not port.is_open:
            raise RuntimeError("Serial port not open")
        port.write(data)
        port.flush()

    async def _read_loop(self) -> None:
        buffer = bytearray()
        while not self._stop.is_set():
            try:
                port = await self._ensure_open()
                chunk = await asyncio.to_thread(port.read, 256)
                if not chunk:
                    continue
                buffer.extend(chunk)
                while b"\x00" in buffer:
                    zero_idx = buffer.index(0)
                    frame = bytes(buffer[: zero_idx + 1])
                    del buffer[: zero_idx + 1]
                    await self._process_frame(frame)
            except Exception as exc:  # noqa: BLE001
                LOGGER.error("Serial read error", extra={"err": str(exc)})
                if self._serial:
                    try:
                        self._serial.close()
                    except Exception:  # noqa: BLE001
                        pass
                await asyncio.sleep(self._config.retry_seconds)

    async def _process_frame(self, frame: bytes) -> None:
        try:
            decoded = cobs.decode(frame)
            payload, crc = decoded[:-2], decoded[-2:]
            if crc16(payload) != int.from_bytes(crc, "big"):
                LOGGER.warning("CRC mismatch", extra={"frame": decoded.hex()})
                return
            message = self._decode(payload)
            await self._handler(message)
        except Exception as exc:  # noqa: BLE001
            LOGGER.warning("Failed to decode frame", extra={"err": str(exc)})

    def _encode(self, payload: dict) -> bytes:
        if self._config.codec == "cbor":
            return cbor_codec.codec.dumps(payload)
        return msgpack.dumps(payload, use_bin_type=True)

    def _decode(self, payload: bytes) -> dict:
        if self._config.codec == "cbor":
            return cbor_codec.codec.loads(payload)
        try:
            return msgpack.loads(payload, raw=False)
        except Exception as exc:  # noqa: BLE001
            LOGGER.debug("Msgpack decode failed, falling back to CBOR", extra={"err": str(exc)})
        return cbor_codec.codec.loads(payload)


def crc16(data: bytes, poly: int = 0x1021, init: int = 0xFFFF) -> int:
    crc = init
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc
