import { config } from './config.js';
import { logger } from './logger.js';
import { publishIncident, publishAck } from './redis_bus.js';

const pending = new Map();

const scheduleTimeout = (entry) => {
  entry.timeoutRef = setTimeout(() => {
    pending.delete(entry.id);
    logger.warn({ correlation_id: entry.id }, 'ACK timeout');
    publishIncident({
      type: 'IOT_TIMEOUT',
      message: `ACK timeout for ${entry.command.asset_id}`,
      correlation_id: entry.id,
      site_id: entry.command.site,
    }).catch((err) => logger.error({ err }, 'Failed to publish incident'));
    if (entry.retries < config.ackRetryLimit) {
      entry.retries += 1;
      entry.retry(entry.command, entry.retries).catch((err) =>
        logger.error({ err }, 'Retry publish failed')
      );
    }
  }, config.ackTimeoutMs);
};

export const trackCommand = (command, retryFn = async () => {}) => {
  if (!command.correlation_id) return;
  const entry = {
    id: command.correlation_id,
    command,
    createdAt: Date.now(),
    retries: 0,
    retry: retryFn,
  };
  pending.set(entry.id, entry);
  scheduleTimeout(entry);
};

export const resolveAck = async (ack) => {
  const entry = pending.get(ack.correlation_id);
  if (entry) {
    clearTimeout(entry.timeoutRef);
    pending.delete(entry.id);
    await publishAck({ ...ack, latency_ms: Date.now() - entry.createdAt });
    logger.info({ ack }, 'ACK resolved');
  } else {
    await publishAck({ ...ack, latency_ms: 0 });
    logger.info({ ack }, 'ACK without pending command');
  }
};

export const clearPending = () => {
  pending.forEach((entry) => clearTimeout(entry.timeoutRef));
  pending.clear();
};
