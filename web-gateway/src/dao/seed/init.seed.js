import bcrypt from 'bcrypt';
import dayjs from 'dayjs';
import { SiteModel } from '../models/site.js';
import { BuildingModel } from '../models/building.js';
import { ZoneModel } from '../models/zone.js';
import { PPModel } from '../models/pp.js';
import { AssetModel } from '../models/assets.js';
import { ResourceModel } from '../models/resources.js';
import { TwinModel } from '../models/twin_model.js';
import { LotModel } from '../models/lot.js';
import { IncubationModel } from '../models/incubation.js';
import { UserModel } from '../models/common.js';
import { seedFieldDefinitions } from './field_defs.seed.js';

const SITE_ID = 'SITE:KIN-GOLIATH';

const makePP = (building, idx, mode) => ({
  pp_id: `${building}-PP-${idx.toString().padStart(2, '0')}`,
  site_id: SITE_ID,
  building_id: building,
  zone_id: `${building === 'BLDG:A' ? 'A' : 'B'}-Z${building === 'BLDG:A' ? idx <= 4 ? 1 : 2 : Math.min(3, Math.ceil(idx / 3))}`,
  mode,
  sensors: [
    { type: 'SHT41', model: 'SHT41', asset_id: 'SENSOR-TEMP-' + idx },
    { type: 'MQ-135', model: 'MQ135', asset_id: 'SENSOR-AIR-' + idx },
    { type: 'CAMERA', model: 'IMX335', asset_id: `CAM-${building}-${idx.toString().padStart(2, '0')}` },
  ],
  actuators: [
    { type: 'LAMP', channel: 'lamp', state: 'AUTO' },
    { type: 'FAN', channel: 'fan', state: 'AUTO' },
    { type: 'HEATER', channel: 'heater', state: 'AUTO' },
    { type: 'DOOR', channel: 'door', state: 'AUTO' },
  ],
  thresholds: {
    mq135_ppm: 250,
    t_c_high: 31,
    t_c_low: 19,
    rh_high: 75,
    rh_low: 40,
  },
});

export const seedInitialData = async () => {
  await seedFieldDefinitions();

  await SiteModel.updateOne(
    { site_id: SITE_ID },
    {
      $set: {
        name: 'Ferme Goliath Kinshasa',
        timezone: 'Africa/Kinshasa',
        gps: { lat: -4.441931, lng: 15.266293 },
        cameras: [
          {
            camera_id: 'CAM-SITE-01',
            label: 'Vue générale',
            rtsp_url: 'rtsp://192.168.10.10/stream',
            onvif: 'http://192.168.10.10/onvif/device_service',
          },
        ],
      },
    },
    { upsert: true }
  );

  try {
    await BuildingModel.collection.dropIndex('buildingId_1');
  } catch (error) {
    if (error?.codeName !== 'IndexNotFound') {
      console.warn('[seed] unable to drop legacy buildingId index', error.message);
    }
  }

  const buildings = [
    {
      building_id: 'BLDG:A',
      site_id: SITE_ID,
      name: 'Poulailler A',
      surface_m2: 70,
      nb_zones: 2,
      usage: 'production',
      cameras: ['CAM-A-01'],
      pondobrood_count: 8,
    },
    {
      building_id: 'BLDG:B',
      site_id: SITE_ID,
      name: 'Poulailler B',
      surface_m2: 90,
      nb_zones: 3,
      usage: 'production',
      cameras: ['CAM-B-01'],
      pondobrood_count: 8,
    },
  ];
  await BuildingModel.bulkWrite(
    buildings.map((b) => ({
      updateOne: {
        filter: { building_id: b.building_id },
        update: { $set: b },
        upsert: true,
      },
    }))
  );

  const zones = [
    { zone_id: 'A-Z1', building_id: 'BLDG:A', site_id: SITE_ID, surface_m2: 35, capacity: 400 },
    { zone_id: 'A-Z2', building_id: 'BLDG:A', site_id: SITE_ID, surface_m2: 35, capacity: 400 },
    { zone_id: 'B-Z1', building_id: 'BLDG:B', site_id: SITE_ID, surface_m2: 30, capacity: 380 },
    { zone_id: 'B-Z2', building_id: 'BLDG:B', site_id: SITE_ID, surface_m2: 30, capacity: 380 },
    { zone_id: 'B-Z3', building_id: 'BLDG:B', site_id: SITE_ID, surface_m2: 30, capacity: 380 },
  ];
  await ZoneModel.bulkWrite(
    zones.map((z) => ({
      updateOne: {
        filter: { zone_id: z.zone_id },
        update: { $set: z },
        upsert: true,
      },
    }))
  );

  const ppDocs = [];
  for (let i = 1; i <= 8; i += 1) {
    ppDocs.push(makePP('BLDG:A', i, i <= 4 ? 'POUSSINIERE' : 'PONDOIR'));
    ppDocs.push(makePP('BLDG:B', i, i <= 4 ? 'POUSSINIERE' : 'PONDOIR'));
  }

  await PPModel.bulkWrite(
    ppDocs.map((pp) => ({
      updateOne: {
        filter: { pp_id: pp.pp_id },
        update: { $set: pp },
        upsert: true,
      },
    }))
  );

  const assets = [
    { asset_id: 'SUP:PROV', site_id: SITE_ID, type: 'AUTRE', label: 'Provenderie', status: 'OK' },
    { asset_id: 'WTR-TNK-01', site_id: SITE_ID, type: 'CUVE', label: 'Cuve 600 L', status: 'OK' },
    { asset_id: 'WTR-PMP-01', site_id: SITE_ID, type: 'POMPE', label: 'Pompe principale', status: 'OK' },
    { asset_id: 'INC-01', site_id: SITE_ID, type: 'INCUBATEUR', label: 'Couveuse 528', status: 'OK' },
    { asset_id: 'PV-INV-01', site_id: SITE_ID, type: 'ENERGIE', label: 'Onduleur solaire', status: 'OK' },
    { asset_id: 'PV-BAT-01', site_id: SITE_ID, type: 'ENERGIE', label: 'Batterie Banque 01', status: 'OK' },
    { asset_id: 'PZEM-A', site_id: SITE_ID, type: 'ENERGIE', label: 'Compteur A', status: 'OK' },
    { asset_id: 'PZEM-B', site_id: SITE_ID, type: 'ENERGIE', label: 'Compteur B', status: 'OK' },
    { asset_id: 'CAM-SITE-01', site_id: SITE_ID, type: 'CAMERA', label: 'Cam Site', status: 'OK' },
    { asset_id: 'CAM-A-01', site_id: SITE_ID, type: 'CAMERA', label: 'Cam Bat A', status: 'OK' },
    { asset_id: 'CAM-B-01', site_id: SITE_ID, type: 'CAMERA', label: 'Cam Bat B', status: 'OK' },
    { asset_id: 'CAM-PROV-01', site_id: SITE_ID, type: 'CAMERA', label: 'Cam Provenderie', status: 'OK' },
  ];

  await AssetModel.bulkWrite(
    assets.map((asset) => ({
      updateOne: {
        filter: { asset_id: asset.asset_id },
        update: { $set: asset },
        upsert: true,
      },
    }))
  );

  const resources = [
    {
      resource_id: 'FEED-MAIZE',
      site_id: SITE_ID,
      category: 'FEED',
      name: 'Maïs concassé',
      stock_level: 1200,
      unit: 'kg',
      reorder_point: 400,
    },
    {
      resource_id: 'MED-VACCIN-01',
      site_id: SITE_ID,
      category: 'MEDICAL',
      name: 'Vaccin Newcastle',
      stock_level: 200,
      unit: 'dose',
      reorder_point: 50,
    },
  ];

  await ResourceModel.bulkWrite(
    resources.map((resource) => ({
      updateOne: {
        filter: { resource_id: resource.resource_id },
        update: { $set: resource },
        upsert: true,
      },
    }))
  );

  await TwinModel.updateOne(
    { site_id: SITE_ID },
    {
      $set: {
        layout_version: 1,
        assets: [
          {
            asset_id: 'BLDG:A',
            type: 'building',
            position: { x: 0, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0 },
            scale: 1,
          },
          {
            asset_id: 'BLDG:B',
            type: 'building',
            position: { x: 80, y: 0, z: 0 },
            rotation: { x: 0, y: Math.PI / 2, z: 0 },
            scale: 1,
          },
          {
            asset_id: 'SUP:PROV',
            type: 'support',
            position: { x: -30, y: 0, z: 10 },
            rotation: { x: 0, y: 0, z: 0 },
            scale: 1,
          },
        ],
      },
    },
    { upsert: true }
  );

  const incubation = {
    incubation_id: 'INC-20231101',
    site_id: SITE_ID,
    incubator_id: 'INC-01',
    eggs_loaded: 520,
    start_date: dayjs().subtract(10, 'day').toDate(),
    expected_hatch_date: dayjs().add(11, 'day').toDate(),
    status: 'EN_COURS',
  };

  await IncubationModel.updateOne(
    { incubation_id: incubation.incubation_id },
    { $setOnInsert: incubation },
    { upsert: true }
  );

  const lot = {
    lot_id: 'LOT-20231015',
    site_id: SITE_ID,
    building_id: 'BLDG:A',
    zone_id: 'A-Z1',
    target_weight_kg: 2.4,
    age_days: 30,
    nb_initial: 400,
    nb_current: 392,
    weight_avg_g: 950,
    kpis: { gmq: 65, fcr: 1.65, mortality: 2, water_per_kg: 4.2, kwh_per_kg: 0.9 },
    status: 'EN_COURS',
    incubations: ['INC-20230910'],
    pp_ids: ['BLDG:A-PP-01', 'BLDG:A-PP-02'],
    timeline: [
      { ts: dayjs().subtract(30, 'day').toDate(), type: 'lot.created', payload: { eggs: 500 } },
      { ts: dayjs().subtract(7, 'day').toDate(), type: 'health.vaccine', payload: { name: 'Newcastle' } },
    ],
  };

  await LotModel.updateOne({ lot_id: lot.lot_id }, { $setOnInsert: lot }, { upsert: true });

  const passwordHash = await bcrypt.hash('ChangeMe123!', 10);
  await UserModel.updateOne(
    { email: 'admin@farmstack.local' },
    {
      $setOnInsert: {
        email: 'admin@farmstack.local',
        phone: '243899000000',
        passwordHash,
        displayName: 'Admin Goliath',
        roles: ['admin'],
        scopes: { site: [SITE_ID] },
      },
    },
    { upsert: true }
  );
};
