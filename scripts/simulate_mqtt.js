#!/usr/bin/env node
const mqtt = require('mqtt');
const { randomUUID } = require('crypto');

const mqttUrl = process.env.MQTT_URL || 'mqtt://localhost:1883';
const site = 'SITE:KIN-GOLIATH';
const device = 'BLDG:A-PP-01';
const client = mqtt.connect(mqttUrl, {
  username: process.env.MQTT_USERNAME || 'farm_iot',
  password: process.env.MQTT_PASSWORD || 'farmstack',
  clientId: `simulator-${Math.random().toString(16).slice(2)}`,
});

client.on('connect', () => {
  console.log('Simulator connected to MQTT');
  setInterval(() => {
    const telemetry = {
      ts: new Date().toISOString(),
      site,
      device,
      asset_id: device,
      metrics: {
        t_c: 22 + Math.random() * 2,
        rh: 55 + Math.random() * 5,
        mq135_ppm: 200 + Math.random() * 20,
      },
      rssi_dbm: -60 + Math.random() * 5,
      idempotency_key: randomUUID(),
    };
    const topic = `v1/farm/${site}/${device}/telemetry/env`;
    client.publish(topic, JSON.stringify(telemetry), { qos: 1 });
  }, 5000);
});

client.on('error', (err) => {
  console.error('MQTT error', err.message);
});
