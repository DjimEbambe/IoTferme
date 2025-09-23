import asyncio
import sys
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_PATH = PROJECT_ROOT / "src"
if str(SRC_PATH) not in sys.path:
    sys.path.insert(0, str(SRC_PATH))

from storage import Storage, StorageConfig  # type: ignore  # noqa: E402


@pytest.fixture(scope="session")
def event_loop():
    loop = asyncio.new_event_loop()
    yield loop
    loop.close()


@pytest.fixture()
async def storage(tmp_path):
    config = StorageConfig(path=tmp_path / "edge.db", retention_days=7)
    store = Storage(config)
    await store.connect()
    yield store
    await store.close()
