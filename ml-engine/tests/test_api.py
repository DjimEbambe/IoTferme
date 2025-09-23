import os

os.environ.setdefault('ML_API_KEY', 'test-key')
os.environ.setdefault('INFLUX_URL', 'http://localhost:8086')
os.environ.setdefault('INFLUX_TOKEN', 'token')
os.environ.setdefault('INFLUX_ORG', 'org')
os.environ.setdefault('MONGO_URI', 'mongodb://localhost:27017/test')
os.environ.setdefault('MODEL_STORE_PATH', '/tmp/farmstack-models')
os.environ.setdefault('REPORT_OUTPUT_PATH', '/tmp/farmstack-reports')

from fastapi.testclient import TestClient
from app.main import app  # noqa: E402

client = TestClient(app)


def test_health_secured():
    response = client.get('/ml/health', headers={'X-API-Key': 'test-key'})
    assert response.status_code == 200
    assert response.json()['status'] == 'ok'


def test_health_rejects_missing_api_key():
    response = client.get('/ml/health')
    assert response.status_code == 422  # missing header
