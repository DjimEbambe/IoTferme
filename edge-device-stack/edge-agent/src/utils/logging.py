"""Logging utilities for the edge agent."""
from __future__ import annotations

import json
import logging
import logging.handlers
from pathlib import Path
from typing import Any


def configure_logging(level: str, log_dir: str, json_mode: bool) -> None:
    """Configure the root logger with rotating file handler and console."""
    log_level = getattr(logging, level.upper(), logging.INFO)
    root = logging.getLogger()
    root.setLevel(log_level)

    Path(log_dir).mkdir(parents=True, exist_ok=True)
    log_path = Path(log_dir) / "edge-agent.log"

    fmt = JSONLogFormatter() if json_mode else logging.Formatter(
        fmt="%(asctime)s | %(levelname)s | %(name)s | %(message)s",
        datefmt="%Y-%m-%dT%H:%M:%S%z",
    )

    file_handler = logging.handlers.RotatingFileHandler(
        log_path, maxBytes=5 * 1024 * 1024, backupCount=5
    )
    file_handler.setFormatter(fmt)

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(fmt)

    root.handlers.clear()
    root.addHandler(file_handler)
    root.addHandler(console_handler)


class JSONLogFormatter(logging.Formatter):
    """JSON formatter for structured logs."""

    def format(self, record: logging.LogRecord) -> str:  # noqa: D401
        payload: dict[str, Any] = {
            "ts": self.formatTime(record, "%Y-%m-%dT%H:%M:%S%z"),
            "level": record.levelname,
            "name": record.name,
            "message": record.getMessage(),
        }
        if record.exc_info:
            payload["exc_info"] = self.formatException(record.exc_info)
        if record.__dict__.get("extra"):
            payload.update(record.__dict__["extra"])
        return json.dumps(payload, ensure_ascii=False)
