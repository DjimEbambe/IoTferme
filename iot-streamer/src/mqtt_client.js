import EventEmitter from 'events';
import mqtt from 'mqtt';
import { config, TOPICS } from './config.js';
import { logger } from './logger.js';

export const mqttEmitter = new EventEmitter();

let client;

export const connectMqtt = () => {
  if (client) return client;
  client = mqtt.connect(config.mqtt.url, {
    username: config.mqtt.username,
    password: config.mqtt.password,
    clientId: config.mqtt.clientId,
    reconnectPeriod: 2000,
    will: {
      topic: config.mqtt.lwtTopic,
      payload: JSON.stringify({ status: 'offline', ts: new Date().toISOString() }),
      qos: 1,
      retain: true,
    },
  });

  client.on('connect', () => {
    logger.info('MQTT connected');
    client.subscribe([TOPICS.TELEMETRY, TOPICS.STATUS, TOPICS.ACK, TOPICS.CMD], { qos: 1 }, (err) => {
      if (err) logger.error({ err }, 'MQTT subscribe failed');
    });
  });

  client.on('message', (topic, buffer) => {
    const payload = buffer.toString();
    try {
      const data = JSON.parse(payload);
      if (topic.includes('/telemetry/')) {
        mqttEmitter.emit('telemetry', { topic, data });
      } else if (topic.endsWith('/status')) {
        mqttEmitter.emit('status', { topic, data });
      } else if (topic.endsWith('/ack')) {
        mqttEmitter.emit('ack', { topic, data });
      } else if (topic.endsWith('/cmd')) {
        mqttEmitter.emit('cmd', { topic, data });
      }
    } catch (error) {
      logger.error({ err: error, topic }, 'Invalid payload JSON');
    }
  });

  client.on('error', (err) => logger.error({ err }, 'MQTT error'));
  client.on('reconnect', () => logger.warn('MQTT reconnecting...'));

  return client;
};
