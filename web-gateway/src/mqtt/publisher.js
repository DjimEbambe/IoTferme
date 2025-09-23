import EventEmitter from 'events';
import mqtt from 'mqtt';
import pino from 'pino';
import { env } from '../config/env.js';
import { v4 as uuid } from 'uuid';

const logger = pino({ name: 'mqtt-publisher', level: 'info' });

const ackBus = new EventEmitter();
ackBus.setMaxListeners(100);

let mqttClient;

const connectMqtt = () => {
  if (mqttClient) return mqttClient;
  mqttClient = mqtt.connect(env.mqtt.url, {
    username: env.mqtt.username,
    password: env.mqtt.password,
    clientId: `web-gateway-${Math.random().toString(16).slice(2)}`,
    reconnectPeriod: 2000,
    queueQoSZero: false,
  });

  mqttClient.on('connect', () => {
    logger.info('Connected to MQTT broker');
    mqttClient.subscribe('v1/farm/+/+/ack', { qos: 1 }, (err) => {
      if (err) {
        logger.error({ err }, 'Failed to subscribe to ACK topic');
      }
    });
  });

  mqttClient.on('message', (topic, buf) => {
    try {
      const payload = JSON.parse(buf.toString());
      if (!payload?.correlation_id) return;
      ackBus.emit(payload.correlation_id, payload);
    } catch (error) {
      logger.error({ err: error }, 'Failed to parse ACK message');
    }
  });

  mqttClient.on('error', (err) => logger.error({ err }, 'MQTT error'));
  mqttClient.on('reconnect', () => logger.warn('MQTT reconnecting'));

  return mqttClient;
};

export const publishCommand = async ({ site, device, asset_id, relays = {}, setpoints = {}, sequence = [] }) => {
  const client = connectMqtt();
  if (!site || !device || !asset_id) {
    throw new Error('site, device and asset_id are required');
  }
  const correlationId = uuid();
  const topic = `v1/farm/${site}/${device}/cmd`;
  const payload = {
    asset_id,
    relays,
    setpoints,
    sequence,
    correlation_id: correlationId,
    ts: new Date().toISOString(),
  };

  const publishPromise = new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      ackBus.removeAllListeners(correlationId);
      reject(new Error('ACK timeout'));
    }, 3000);

    ackBus.once(correlationId, (ack) => {
      clearTimeout(timeout);
      if (ack.ok) {
        resolve(ack);
      } else {
        reject(new Error(ack.message || 'Command rejected'));
      }
    });
  });

  await new Promise((resolve, reject) => {
    client.publish(topic, JSON.stringify(payload), { qos: 1 }, (err) => {
      if (err) reject(err);
      else resolve();
    });
  });

  logger.info({ topic, correlationId, asset_id }, 'Command published');
  return publishPromise;
};
