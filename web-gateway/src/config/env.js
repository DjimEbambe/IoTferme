import dotenv from 'dotenv';

if (!process.env.NODE_ENV || process.env.NODE_ENV === 'development') {
  dotenv.config();
}

const required = (key, fallback) => {
  const value = process.env[key] ?? fallback;
  if (value === undefined) {
    throw new Error(`Missing required env var ${key}`);
  }
  return value;
};

export const env = {
  nodeEnv: process.env.NODE_ENV || 'development',
  port: parseInt(process.env.PORT || '8000', 10),
  baseUrl: process.env.BASE_URL || 'http://localhost:8000',
  mongoUri: required('MONGO_URI'),
  mongoUser: process.env.MONGO_USER,
  mongoPassword: process.env.MONGO_PASSWORD,
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
    windowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS || '60000', 10),
    max: parseInt(process.env.RATE_LIMIT_MAX || '120', 10),
  },
  timezone: process.env.TZ || 'Africa/Kinshasa',
  cspNonceSecret: required('CSP_NONCE_SECRET'),
  rbacDefaultRole: process.env.RBAC_DEFAULT_ROLE || 'visitor',
};

export const isProd = env.nodeEnv === 'production';
export const isTest = env.nodeEnv === 'test';
