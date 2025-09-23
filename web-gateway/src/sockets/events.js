import { getRedisPubSub } from '../redis.js';
import { logger } from '../logger.js';

const CHANNEL_TELEMETRY = 'iot:telemetry';
const CHANNEL_KPI = 'kpi:update';
const CHANNEL_INCIDENT = 'incident:new';
const CHANNEL_CMD_ACK = 'cmd:ack';

export const registerRedisBridge = (io) => {
  const subscriber = getRedisPubSub();
  const forward = (channel, payload) => {
    try {
      const msg = JSON.parse(payload);
      const scopeRoom = msg.room || msg.asset_id ? `asset:${msg.asset_id}` : `site:${msg.site_id || 'SITE:KIN-GOLIATH'}`;
      switch (channel) {
        case CHANNEL_TELEMETRY:
          io.to(scopeRoom).emit('telemetry.update', msg);
          break;
        case CHANNEL_KPI:
          io.to(scopeRoom).emit('kpi.update', msg);
          break;
        case CHANNEL_INCIDENT:
          io.to(scopeRoom).emit('incident.new', msg);
          break;
        case CHANNEL_CMD_ACK:
          io.to(scopeRoom).emit('cmd.ack', msg);
          break;
        default:
          break;
      }
    } catch (error) {
      logger.error({ err: error, channel }, 'Redis bridge failed to parse payload');
    }
  };

  subscriber.subscribe(CHANNEL_TELEMETRY, (payload) => forward(CHANNEL_TELEMETRY, payload));
  subscriber.subscribe(CHANNEL_KPI, (payload) => forward(CHANNEL_KPI, payload));
  subscriber.subscribe(CHANNEL_INCIDENT, (payload) => forward(CHANNEL_INCIDENT, payload));
  subscriber.subscribe(CHANNEL_CMD_ACK, (payload) => forward(CHANNEL_CMD_ACK, payload));

  logger.info('Socket bridge subscribed to Redis channels');
};
