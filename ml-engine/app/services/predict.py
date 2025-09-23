from datetime import timedelta
from typing import List

import numpy as np
from pandas import Series

from ..models.predict import PredictRequest, PredictPoint


def forecast(req: PredictRequest) -> List[PredictPoint]:
    if len(req.series) < 3:
        raise ValueError('Need at least 3 points for prediction')

    values = Series([p.value for p in req.series])
    ma = values.rolling(window=min(5, len(values)), min_periods=1).mean()
    last_ts = req.series[-1].ts
    interval = timedelta(hours=req.interval_hours)
    last_value = float(ma.iloc[-1])

    trend = (ma.iloc[-1] - ma.iloc[max(len(ma) - 2, 0)]) if len(ma) > 1 else 0
    trend_per_step = trend / req.interval_hours if req.interval_hours else 0

    forecast_points: List[PredictPoint] = []
    for h in range(1, req.horizon + 1):
        ts = last_ts + interval * h
        value = last_value + trend_per_step * h
        value = max(value, 0)
        forecast_points.append(PredictPoint(ts=ts, value=float(np.round(value, 3))))
    return forecast_points
