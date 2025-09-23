import pino from 'pino';

export const logger = pino({
  name: 'farmstack-iot-streamer',
  level: process.env.LOG_LEVEL || 'info',
});
