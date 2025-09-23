import { writeTelemetry, flushWrites } from './influx_writer.js';
import { pushTelemetry, publishIncident } from './redis_bus.js';
import { resolveAck, trackCommand, clearPending } from './correlator.js';
import { logger } from './logger.js';
import { config } from './config.js';

import Ajv from 'ajv';
import addFormats from 'ajv-formats';
import { createRequire } from 'module';
import { mqttEmitter, connectMqtt } from './mqtt_client.js';

const require = createRequire(import.meta.url);
const telemetrySchema = require('./validators/telemetry.schema.json');
const ackSchema = require('./validators/ack.schema.json');

const ajv = new Ajv({ allErrors: true, coerceTypes: true, strict: false, allowUnionTypes: true });
addFormats(ajv);
const validateTelemetry = ajv.compile(telemetrySchema);
const validateAck = ajv.compile(ackSchema);

const mqttClient = connectMqtt();

mqttEmitter.on('telemetry', async ({ topic, data }) => {
  if (!validateTelemetry(data)) {
    logger.warn({ errors: validateTelemetry.errors, topic }, 'Telemetry rejected');
    return;
  }
  try {
    await writeTelemetry({ topic, data });
    await pushTelemetry({ topic, ...data });
  } catch (error) {
    logger.error({ err: error }, 'Failed processing telemetry');
  }
});

mqttEmitter.on('ack', ({ data }) => {
  if (!validateAck(data)) {
    logger.warn({ data }, 'Invalid ACK');
    return;
  }
  resolveAck(data).catch((error) => {
    logger.error({ err: error, correlation_id: data.correlation_id }, 'Failed to process ACK');
  });
});

mqttEmitter.on('cmd', ({ topic, data }) => {
  if (!data.correlation_id) return;
  trackCommand(data, async (command) => {
    const republishTopic = topic;
    mqttClient.publish(republishTopic, JSON.stringify(command), { qos: 1 }, (err) => {
      if (err) {
        logger.error({ err }, 'Failed to republish command');
      } else {
        logger.info({ correlation_id: command.correlation_id }, 'Command retried');
      }
    });
  });
});

mqttEmitter.on('status', ({ data }) => {
  if (data.status === 'offline') {
    publishIncident({
      type: 'DEVICE_OFFLINE',
      message: `${data.device || 'device'} offline`,
      device: data.device,
      site_id: data.site,
    }).catch((err) => logger.error({ err, device: data.device }, 'Failed to publish incident'));
  }
});

const gracefulShutdown = async () => {
  logger.info('Shutting down...');
  try {
    await flushWrites();
    clearPending();
    mqttClient.end();
  } finally {
    process.exit(0);
  }
};

process.on('SIGINT', gracefulShutdown);
process.on('SIGTERM', gracefulShutdown);

process.on('unhandledRejection', (reason) => {
  logger.error({ err: reason }, 'Unhandled promise rejection');
});

process.on('uncaughtException', (error) => {
  logger.error({ err: error }, 'Uncaught exception');
  process.exit(1);
});

logger.info({ mqtt: config.mqtt.url, redis: config.redis.url }, 'iot-streamer started');
