import logging
from apscheduler.schedulers.asyncio import AsyncIOScheduler
from apscheduler.triggers.cron import CronTrigger

logger = logging.getLogger('farmstack-ml-scheduler')

scheduler = AsyncIOScheduler(timezone='Africa/Kinshasa')


async def training_job():
    logger.info('Running daily training job (placeholder)')


async def report_job():
    logger.info('Generating performance report (placeholder)')


def configure_scheduler():
    if scheduler.running:
        return scheduler
    scheduler.add_job(training_job, CronTrigger(hour=2, minute=0))
    scheduler.add_job(report_job, CronTrigger(hour=18, minute=0))
    scheduler.start()
    return scheduler
