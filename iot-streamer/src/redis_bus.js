import { createClient } from 'redis';
import { config } from './config.js';
import { logger } from './logger.js';

let redisClient;
let publisher;

const ensureClient = async () => {
  if (!redisClient) {
    redisClient = createClient({ url: config.redis.url });
    redisClient.on('error', (err) => logger.error({ err }, 'Redis client error'));
    await redisClient.connect();
  }
  if (!publisher) {
    publisher = redisClient.duplicate();
    publisher.on('error', (err) => logger.error({ err }, 'Redis publisher error'));
    await publisher.connect();
  }
};

export const pushTelemetry = async (payload) => {
  await ensureClient();
  const json = JSON.stringify(payload);
  await redisClient.xAdd(config.redis.streamKey, '*', { payload: json });
  await publisher.publish(config.redis.pubChannel, json);
};

export const publishAck = async (payload) => {
  await ensureClient();
  await publisher.publish('cmd:ack', JSON.stringify(payload));
};

export const publishKpiUpdate = async (payload) => {
  await ensureClient();
  await publisher.publish('kpi:update', JSON.stringify(payload));
};

export const publishIncident = async (payload) => {
  await ensureClient();
  await publisher.publish('incident:new', JSON.stringify(payload));
};

export const getRedisHealth = async () => {
  await ensureClient();
  const pong = await redisClient.ping();
  return pong === 'PONG';
};
