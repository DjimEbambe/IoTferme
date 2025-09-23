import { createClient } from 'redis';
import { env } from './config/env.js';
import { logger } from './logger.js';

let client;
let pubSub;

export const getRedisClient = () => {
  if (!client) {
    client = createClient({ url: env.redisUrl });
    client.on('error', (err) => logger.error({ err }, 'Redis client error'));
    client.on('connect', () => logger.info('Redis connected'));
    client.connect().catch((err) => logger.error({ err }, 'Redis connect error'));
  }
  return client;
};

export const getRedisPubSub = () => {
  if (!pubSub) {
    pubSub = getRedisClient().duplicate();
    pubSub.on('error', (err) => logger.error({ err }, 'Redis pubsub error'));
    pubSub.connect().catch((err) => logger.error({ err }, 'Redis pubsub connect error'));
  }
  return pubSub;
};
