import { InfluxDBClient } from '@influxdata/influxdb3-client';
import { env } from '../config/env.js';

const clients = new Map();

const makeClient = (database) =>
  new InfluxDBClient({
    host: env.influx.url,
    token: env.influx.token,
    database,
  });

export const getInfluxClient = (database = env.influx.bucketRaw) => {
  if (!clients.has(database)) {
    clients.set(database, makeClient(database));
  }
  return clients.get(database);
};

export const influxDatabases = {
  raw: env.influx.bucketRaw,
  agg1m: env.influx.bucket1m,
  agg5m: env.influx.bucket5m,
};
