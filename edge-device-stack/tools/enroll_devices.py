#!/usr/bin/env python3
"""Utility to open a pairing window on the ESP32 gateway and watch joining devices."""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover
    raise SystemExit("pyserial is required: pip install pyserial") from exc

REPO_ROOT = Path(__file__).resolve().parents[1]
SYS_PATH = REPO_ROOT / "edge-agent" / "src"
if str(SYS_PATH) not in sys.path:
    sys.path.insert(0, str(SYS_PATH))

from utils import cobs  # type: ignore
from serial_bridge import crc16  # type: ignore


def encode(payload: dict) -> bytes:
    raw = json.dumps(payload).encode("utf-8")
    frame = raw + crc16(raw).to_bytes(2, "big")
    encoded = cobs.encode(frame)
    return encoded + b"\x00"


def decode(buffer: bytes) -> dict | None:
    try:
        decoded = cobs.decode(buffer)
        if len(decoded) < 3:
            return None
        raw, crc_bytes = decoded[:-2], decoded[-2:]
        if crc16(raw) != int.from_bytes(crc_bytes, "big"):
            return None
        return json.loads(raw.decode("utf-8"))
    except Exception:
        return None


def run(device: str, duration: int) -> None:
    with serial.Serial(device, baudrate=921600, timeout=0.2) as port:
        print(f"Opening pairing window {duration}s on {device}")
        port.write(encode({"type": "pair_begin", "duration_s": duration}))
        port.flush()
        deadline = time.time() + duration + 5
        buffer = bytearray()
        while time.time() < deadline:
            chunk = port.read(256)
            if not chunk:
                continue
            buffer.extend(chunk)
            while b"\x00" in buffer:
                idx = buffer.index(0)
                frame = bytes(buffer[:idx])
                del buffer[: idx + 1]
                payload = decode(frame)
                if payload:
                    print(f"<-- {json.dumps(payload)}")
        print("Pairing window closed")
        port.write(encode({"type": "pair_end"}))
        port.flush()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Enroll ESP-NOW devices via the gateway")
    parser.add_argument("--device", default="/dev/ttyESP-GW", help="Serial device of the gateway")
    parser.add_argument("--duration", type=int, default=120, help="Pairing window duration in seconds")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    run(args.device, args.duration)
