import dotenv from 'dotenv';

if (!process.env.NODE_ENV || process.env.NODE_ENV === 'development') {
  dotenv.config();
}

const stageName = (process.env.IOTFARM_ENV || process.env.NODE_ENV || 'development').toLowerCase();
const stageToken = stageName.replace(/[^a-z0-9]/gi, '_').toUpperCase();

const resolveStageValue = (key) => {
  const stageKey = `${key}_${stageToken}`;
  if (process.env[stageKey] !== undefined) return process.env[stageKey];
  return process.env[key];
};

const required = (key, fallback) => {
  const value = resolveStageValue(key);
  if (value !== undefined && value !== '') return value;
  if (fallback !== undefined) return fallback;
  const stageKey = `${key}_${stageToken}`;
  throw new Error(`Missing required env var ${key} (stage override checked: ${stageKey})`);
};

const optional = (key, fallback) => {
  const value = resolveStageValue(key);
  if (value !== undefined) return value;
  return fallback;
};

const requiredNumber = (key, fallback) => {
  const raw = required(key, fallback);
  const parsed = Number(raw);
  if (Number.isNaN(parsed)) {
    throw new Error(`Invalid number for env var ${key}`);
  }
  return parsed;
};

export const env = {
  nodeEnv: process.env.NODE_ENV || 'development',
  stage: stageName,
  port: requiredNumber('PORT', 8000),
  baseUrl: optional('BASE_URL', 'http://localhost:8000'),
  mongoUri: required('MONGO_URI'),
  mongoUser: optional('MONGO_USER'),
  mongoPassword: optional('MONGO_PASSWORD'),
  redisUrl: required('REDIS_URL'),
  sessionSecret: required('SESSION_SECRET'),
  csrfSecret: required('CSRF_SECRET'),
  influx: {
    url: required('INFLUX_URL'),
    org: required('INFLUX_ORG'),
    token: required('INFLUX_TOKEN'),
    bucketRaw: required('INFLUX_BUCKET_RAW'),
    bucket1m: required('INFLUX_BUCKET_1M'),
    bucket5m: required('INFLUX_BUCKET_5M'),
  },
  mqtt: {
    url: required('MQTT_URL'),
    username: process.env.MQTT_USERNAME,
    password: process.env.MQTT_PASSWORD,
  },
  ml: {
    baseUrl: required('ML_API_URL'),
    apiKey: required('ML_API_KEY'),
  },
  rateLimit: {
    windowMs: requiredNumber('RATE_LIMIT_WINDOW_MS', 60000),
    max: requiredNumber('RATE_LIMIT_MAX', 120),
  },
  timezone: optional('TZ', 'Africa/Kinshasa'),
  cspNonceSecret: required('CSP_NONCE_SECRET'),
  rbacDefaultRole: optional('RBAC_DEFAULT_ROLE', 'visitor'),
};

export const isProd = env.nodeEnv === 'production';
export const isTest = env.nodeEnv === 'test';
