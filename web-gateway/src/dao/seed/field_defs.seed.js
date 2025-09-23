import dayjs from 'dayjs';
import { FieldDefModel } from '../models/field_def.js';

const now = () => dayjs().toDate();

const definitions = [
  {
    scope: 'site',
    key: 'vent_type',
    label: 'Type de ventilation',
    type: 'string',
    enum_values: ['NATURELLE', 'FORCEE'],
    required: true,
  },
  {
    scope: 'building',
    key: 'bedding_type',
    label: 'Litière',
    type: 'string',
    enum_values: ['COPEAUX', 'BALLERES', 'SABLE'],
  },
  {
    scope: 'pp',
    key: 'lamp_power_w',
    label: 'Puissance lampe (W)',
    type: 'number',
    min: 10,
    max: 200,
    unit: 'W',
    required: true,
  },
  {
    scope: 'pp',
    key: 'target_lux',
    label: 'Lux cible',
    type: 'number',
    min: 50,
    max: 300,
    unit: 'lux',
  },
  {
    scope: 'pp',
    key: 'min_operating_level_l',
    label: 'Niveau min eau (L)',
    type: 'number',
    min: 0,
    max: 10,
    unit: 'L',
  },
  {
    scope: 'asset',
    key: 'turn_angle_deg',
    label: 'Angle rotation',
    type: 'number',
    min: 0,
    max: 360,
    unit: 'deg',
  },
  {
    scope: 'resource',
    key: 'supplier_name',
    label: 'Fournisseur',
    type: 'string',
  },
  {
    scope: 'twin',
    key: 'mesh_template',
    label: 'Template Mesh',
    type: 'string',
  },
  {
    scope: 'lot',
    key: 'feed_formula',
    label: 'Formule alimentation',
    type: 'string',
  },
  {
    scope: 'zone',
    key: 'density_limit',
    label: 'Limite densité (kg/m2)',
    type: 'number',
    min: 5,
    max: 50,
  },
];

export const seedFieldDefinitions = async () => {
  const ops = definitions.map((def) => ({
    updateOne: {
      filter: { scope: def.scope, key: def.key, version: def.version || 1 },
      update: {
        $setOnInsert: { createdAt: now() },
        $set: {
          ...def,
          effective_from: def.effective_from || now(),
          updatedAt: now(),
        },
      },
      upsert: true,
    },
  }));
  if (ops.length > 0) {
    await FieldDefModel.bulkWrite(ops);
  }
};
