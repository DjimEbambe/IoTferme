import numpy as np
from ..models.anomaly import AnomalyRequest, AnomalyResult
from ..models.common import SeriesPoint


def _zscore(values: np.ndarray, sensitivity: float):
    mean = np.mean(values)
    std = np.std(values)
    if std == 0:
        return []
    z = (values - mean) / std
    idx = np.where(np.abs(z) > sensitivity)[0]
    return idx, z


def _iqr(values: np.ndarray, sensitivity: float):
    q1 = np.percentile(values, 25)
    q3 = np.percentile(values, 75)
    iqr = q3 - q1
    lower = q1 - sensitivity * iqr
    upper = q3 + sensitivity * iqr
    idx = np.where((values < lower) | (values > upper))[0]
    return idx, None


def _changepoint(values: np.ndarray, sensitivity: float):
    diffs = np.abs(np.diff(values, prepend=values[0]))
    threshold = np.mean(diffs) + sensitivity * np.std(diffs)
    idx = np.where(diffs > threshold)[0]
    return idx, diffs


METHODS = {
    'zscore': _zscore,
    'iqr': _iqr,
    'changepoint': _changepoint,
}


def detect_anomalies(req: AnomalyRequest):
    if not req.series:
        return []
    values = np.array([point.value for point in req.series], dtype=float)
    fn = METHODS[req.method]
    idx, score = fn(values, req.sensitivity)
    anomalies = []
    for i in idx:
        point = req.series[i]
        anomalies.append(
            AnomalyResult(
                index=int(i),
                point=SeriesPoint(ts=point.ts, value=point.value),
                score=float(score[i]) if score is not None else 0.0,
            )
        )
    return anomalies
