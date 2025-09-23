import { InfluxDBClient } from '@influxdata/influxdb3-client';
import { config } from './config.js';
import { logger } from './logger.js';

let influxClient;

const getClient = () => {
  if (!influxClient) {
    influxClient = new InfluxDBClient({
      host: config.influx.url,
      token: config.influx.token,
      database: config.influx.bucket,
    });
  }
  return influxClient;
};

const measurementMap = (suffix) => {
  switch (suffix) {
    case 'env':
      return 'env';
    case 'power':
      return 'power';
    case 'water':
      return 'water';
    case 'incubator':
      return 'incubator';
    default:
      return 'env';
  }
};

export const writeTelemetry = async ({ topic, data }) => {
  const segments = topic.split('/');
  if (segments.length < 6) return;
  const site = segments[2];
  const device = segments[3];
  const scope = segments[5];
  const measurement = measurementMap(scope);

  const fields = {};
  if (data.metrics) {
    Object.entries(data.metrics).forEach(([key, value]) => {
      if (typeof value === 'number') {
        fields[key] = value;
      }
    });
  }
  if (typeof data.rssi_dbm === 'number') {
    fields.rssi_dbm = data.rssi_dbm;
  }
  if (!Object.keys(fields).length) return;

  const timestamp = (() => {
    const parsed = data.ts ? Date.parse(data.ts) : Date.now();
    return Number.isNaN(parsed) ? new Date() : new Date(parsed);
  })();

  const point = {
    measurement,
    tags: {
      site_id: site,
      device,
      asset_id: data.asset_id || device,
      sensor: scope,
    },
    fields,
    timestamp,
  };

  try {
    await getClient().write(point);
  } catch (error) {
    logger.error({ err: error }, 'Failed to write to InfluxDB 3');
    throw error;
  }
};

export const flushWrites = async () => Promise.resolve();
