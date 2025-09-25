import dayjs from 'dayjs';
import { getInfluxClient, influxDatabases } from './client.js';

const unitMap = {
  s: 'SECOND',
  m: 'MINUTE',
  h: 'HOUR',
  d: 'DAY',
};

const toInterval = (range = '24h') => {
  const match = /^([0-9]+)([smhd])$/i.exec(range.trim());
  if (!match) return "INTERVAL '24 HOUR'";
  const [, value, unit] = match;
  return `INTERVAL '${value} ${unitMap[unit.toLowerCase()]}'`;
};

const aggFn = (aggregate) => {
  switch ((aggregate || '').toLowerCase()) {
    case 'sum':
      return 'SUM';
    case 'max':
      return 'MAX';
    case 'min':
      return 'MIN';
    default:
      return 'AVG';
  }
};

const sanitizeIdent = (value) => value.replace(/[^-a-zA-Z0-9_:]/g, '');
const sanitizeLiteral = (value) => value.replace(/'/g, "''");

export const fetchAssetMetricSeries = async ({ metric, assetId, range = '24h' }) => {
  const database = influxDatabases.agg1m || influxDatabases.raw;
  const client = getInfluxClient(database);
  const measurement = metric.includes('energy') ? 'power' : metric.includes('water') ? 'water' : 'env';
  const intervalExpr = toInterval(range);
  const metricIdent = sanitizeIdent(metric);
  const sql = `SELECT time, "${metricIdent}" AS value
    FROM "${sanitizeIdent(measurement)}"
    WHERE asset_id = '${sanitizeLiteral(assetId)}'
      AND time >= now() - ${intervalExpr}
    ORDER BY time ASC`;

  const rows = [];
  try {
    const iterator = client.query(sql);
    // eslint-disable-next-line no-restricted-syntax
    for await (const row of iterator) {
      rows.push({ ts: row.time?.toISOString?.() || row.time, value: Number(row.value ?? 0) });
    }
  } catch (error) {
    return [];
  }
  return rows;
};

export const fetchLatestTelemetry = async ({ assetId, measurement = 'env', fields = [] }) => {
  const client = getInfluxClient(influxDatabases.raw);
  const sql = `SELECT * FROM "${sanitizeIdent(measurement)}"
    WHERE asset_id = '${sanitizeLiteral(assetId)}'
    ORDER BY time DESC
    LIMIT 1`;
  const telemetry = {};
  try {
    const iterator = client.query(sql);
    const { value: row } = await iterator.next();
    if (row) {
      const wanted = fields.length ? fields : Object.keys(row).filter((key) => !['time', 'asset_id', 'site_id', 'building_id', 'zone_id'].includes(key));
      wanted.forEach((field) => {
        if (row[field] !== undefined) {
          telemetry[field] = { value: Number(row[field]), ts: row.time?.toISOString?.() || row.time };
        }
      });
    }
  } catch (error) {
    return {};
  }
  return telemetry;
};

export const fetchRangeAggregates = async ({
  siteId,
  measurement = 'env',
  field,
  start,
  stop,
  aggregate = 'mean',
}) => {
  const client = getInfluxClient(influxDatabases.agg1m || influxDatabases.raw);
  const agg = aggFn(aggregate);
  const startIso = dayjs(start).toISOString();
  const stopIso = dayjs(stop).toISOString();
  const fieldIdent = sanitizeIdent(field);
  const sql = `SELECT date_bin(INTERVAL '1 hour', time) AS bucket, ${agg}("${fieldIdent}") AS value
    FROM "${sanitizeIdent(measurement)}"
    WHERE site_id = '${sanitizeLiteral(siteId)}'
      AND time BETWEEN '${startIso}' AND '${stopIso}'
    GROUP BY bucket
    ORDER BY bucket ASC`;

  const result = [];
  try {
    const iterator = client.query(sql);
    // eslint-disable-next-line no-restricted-syntax
    for await (const row of iterator) {
      result.push({ ts: row.bucket?.toISOString?.() || row.bucket, value: Number(row.value ?? 0) });
    }
  } catch (error) {
    return [];
  }
  return result;
};
