from datetime import datetime, timedelta
from ..models.schedule import ScheduleRequest, TaskItem, ScheduleResponse


def build_schedule(req: ScheduleRequest) -> ScheduleResponse:
    start = datetime.utcnow()
    tasks = []
    for offset in range(0, req.horizon_hours, 6):
        ts = start + timedelta(hours=offset)
        action = 'Inspection visuelle'
        notes = None
        if offset % 12 == 0:
            action = 'Contrôle alimentation'
        if req.current_age_days < 7:
            notes = 'Renforcer chauffage' if action != 'Contrôle alimentation' else 'Ajouter starter riche protéines'
        tasks.append(TaskItem(ts=ts.isoformat(), action=action, target=req.lot_id, notes=notes))
    return ScheduleResponse(tasks=tasks)
