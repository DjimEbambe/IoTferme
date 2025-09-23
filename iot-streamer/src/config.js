import dotenv from 'dotenv';

if (!process.env.NODE_ENV || process.env.NODE_ENV === 'development') {
  dotenv.config();
}

const required = (key, fallback) => {
  const value = process.env[key] ?? fallback;
  if (value === undefined) throw new Error(`Missing env ${key}`);
  return value;
};

export const config = {
  mqtt: {
    url: required('MQTT_URL'),
    username: process.env.MQTT_USERNAME,
    password: process.env.MQTT_PASSWORD,
    clientId: process.env.MQTT_CLIENT_ID || `iot-streamer-${Math.random().toString(16).slice(2)}`,
    lwtTopic: process.env.MQTT_LWT_TOPIC || 'v1/farm/SITE:KIN-GOLIATH/system/status',
  },
  influx: {
    url: required('INFLUX_URL'),
    token: required('INFLUX_TOKEN'),
    org: required('INFLUX_ORG'),
    bucket: required('INFLUX_BUCKET_RAW'),
  },
  redis: {
    url: required('REDIS_URL'),
    streamKey: process.env.REDIS_STREAM || 'telemetry_buffer',
    pubChannel: process.env.REDIS_PUBSUB_CHANNEL || 'iot:telemetry',
  },
  ackTimeoutMs: parseInt(process.env.ACK_TIMEOUT_MS || '3000', 10),
  ackRetryLimit: parseInt(process.env.ACK_RETRY_LIMIT || '3', 10),
};

export const TOPICS = {
  TELEMETRY: 'v1/farm/+/+/telemetry/+',
  STATUS: 'v1/farm/+/+/status',
  ACK: 'v1/farm/+/+/ack',
  CMD: 'v1/farm/+/+/cmd',
};
