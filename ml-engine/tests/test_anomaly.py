from datetime import datetime, timedelta

from app.models.anomaly import AnomalyRequest
from app.models.common import SeriesPoint
from app.services.anomaly import detect_anomalies


def test_detect_anomaly_zscore():
    base = datetime.utcnow()
    series = [SeriesPoint(ts=base + timedelta(minutes=i), value=20.0) for i in range(10)]
    series.append(SeriesPoint(ts=base + timedelta(minutes=10), value=50.0))
    req = AnomalyRequest(series=series, method='zscore', sensitivity=2.0)
    result = detect_anomalies(req)
    assert len(result) == 1
    assert result[0].point.value == 50.0
