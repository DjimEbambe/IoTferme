#!/usr/bin/env python3
"""Minimal helper to host a firmware binary and send an OTA trigger through the gateway."""
from __future__ import annotations

import argparse
import base64
import hashlib
import http.server
import json
import os
import socket
import threading
import time
from pathlib import Path

import serial  # type: ignore

REPO_ROOT = Path(__file__).resolve().parents[1]
SYS_PATH = REPO_ROOT / "edge-agent" / "src"
import sys
if str(SYS_PATH) not in sys.path:
    sys.path.insert(0, str(SYS_PATH))

from utils import cobs  # type: ignore
from serial_bridge import crc16  # type: ignore


def encode(payload: dict) -> bytes:
    raw = json.dumps(payload).encode("utf-8")
    frame = raw + crc16(raw).to_bytes(2, "big")
    encoded = cobs.encode(frame)
    return encoded + b"\x00"


def sha256_hex(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(4096), b""):
            digest.update(chunk)
    return digest.hexdigest()


def start_http_server(directory: Path, port: int) -> threading.Thread:
    os.chdir(directory)
    handler = http.server.SimpleHTTPRequestHandler
    server = http.server.ThreadingHTTPServer(("0.0.0.0", port), handler)

    def run() -> None:
        with server:
            server.serve_forever()

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return thread


def detect_host_ip() -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("8.8.8.8", 80))
        ip = sock.getsockname()[0]
    finally:
        sock.close()
    return ip


def main() -> None:
    parser = argparse.ArgumentParser(description="OTA demo via USB gateway")
    parser.add_argument("firmware", type=Path, help="Firmware binary (.bin)")
    parser.add_argument("asset", help="Asset ID (ex: A-PP-01)")
    parser.add_argument("--device", default="/dev/ttyESP-GW")
    parser.add_argument("--port", type=int, default=8085, help="HTTP port for OTA hosting")
    args = parser.parse_args()

    firmware = args.firmware.resolve()
    if not firmware.exists():
        raise SystemExit(f"Firmware not found: {firmware}")

    directory = firmware.parent
    thread = start_http_server(directory, args.port)
    time.sleep(0.5)
    host_ip = detect_host_ip()
    url = f"http://{host_ip}:{args.port}/{firmware.name}"
    sha256 = sha256_hex(firmware)
    print(f"Serving {firmware.name} at {url}")

    ota_payload = {
        "type": "cmd",
        "asset_id": args.asset,
        "ota": {
            "url": url,
            "sha256": sha256,
        },
        "correlation_id": f"ota-{int(time.time())}",
    }

    with serial.Serial(args.device, baudrate=921600, timeout=1.0) as port:
        port.write(encode(ota_payload))
        port.flush()
        print(f"OTA trigger sent to {args.asset}. Keep this process running until completion.")
        try:
            while thread.is_alive():
                time.sleep(1)
        except KeyboardInterrupt:
            print("Stopping server...")


if __name__ == "__main__":
    main()
