import { InfluxDBClient } from '@influxdata/influxdb3-client';
import { connectMqtt } from './mqtt_client.js';
import { config } from './config.js';
import { getRedisHealth } from './redis_bus.js';

const checkInflux = async () => {
  try {
    const client = new InfluxDBClient({
      host: config.influx.url,
      token: config.influx.token,
      database: config.influx.bucket,
    });
    const iterator = client.query('SELECT 1');
    await iterator.next();
    await client.close?.();
    return true;
  } catch (error) {
    return false;
  }
};

export const getHealth = async () => {
  const mqtt = connectMqtt();
  const redisOk = await getRedisHealth().catch(() => false);
  const influxOk = await checkInflux();
  return {
    status: mqtt.connected && redisOk && influxOk ? 'ok' : 'degraded',
    mqtt: mqtt.connected,
    redis: redisOk,
    influx: influxOk,
    timestamp: new Date().toISOString(),
  };
};
