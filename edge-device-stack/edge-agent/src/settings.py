"""Runtime settings loaded from environment."""
from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Literal

from pydantic import AnyUrl, Field, SecretStr
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    site: str = Field(default="KIN-GOLIATH")
    device_id: str = Field(default="esp32gw-01")

    mqtt_uri: AnyUrl = Field(default="mqtts://broker.example.com:8883")
    mqtt_username: str = Field(default="edge-agent")
    mqtt_password: SecretStr = Field(default=SecretStr("change-me"))
    mqtt_keepalive: int = Field(default=30)
    mqtt_use_tls: bool = Field(default=True)
    mqtt_ca_file: Path | None = Field(default=Path("/etc/ssl/certs/ca-certificates.crt"))
    mqtt_cert_file: Path | None = None
    mqtt_key_file: Path | None = None
    mqtt_qos: int = Field(default=1, ge=0, le=1)
    mqtt_lwt_topic: str | None = None
    mqtt_lwt_payload: str | None = None

    usb_device: Path = Field(default=Path("/dev/ttyESP-GW"))
    serial_baud: int = Field(default=921600)
    serial_retry_seconds: int = Field(default=5)
    serial_codec: Literal["cbor", "msgpack"] = Field(default="msgpack")

    sqlite_path: Path = Field(default=Path("/var/lib/edge-agent/edge.db"))
    retention_days: int = Field(default=28)
    backlog_max_batch: int = Field(default=500)
    backlog_max_rate: int = Field(default=500)

    edge_bind_host: str = Field(default="127.0.0.1")
    edge_bind_port: int = Field(default=8081)
    edge_bind_lan: bool = Field(default=False)
    edge_basic_auth_user: str = Field(default="admin")
    edge_basic_auth_pass: SecretStr = Field(default=SecretStr("change-me"))

    time_sync_interval_hours: int = Field(default=6)
    cmd_timeout_seconds: int = Field(default=3)
    cmd_max_retries: int = Field(default=2)
    cmd_retry_backoff_seconds: int = Field(default=2)

    log_level: str = Field(default="INFO")
    log_json: bool = Field(default=True)
    log_dir: Path = Field(default=Path("/var/log/edge-agent"))

    model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8", extra="allow")

    @property
    def base_topic(self) -> str:
        return f"v1/farm/{self.site}/{self.device_id}"

    @property
    def telemetry_topics(self) -> dict[str, str]:
        return {
            key: f"{self.base_topic}/telemetry/{key}"
            for key in ("env", "power", "water", "incubator")
        }

    @property
    def cmd_topic(self) -> str:
        return f"{self.base_topic}/cmd"

    @property
    def ack_topic(self) -> str:
        return f"{self.base_topic}/ack"

    @property
    def status_topic(self) -> str:
        return f"{self.base_topic}/status"


@lru_cache(maxsize=1)
def get_settings() -> Settings:
    return Settings()
