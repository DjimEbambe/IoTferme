import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const SensorSchema = new Schema(
  {
    type: { type: String, required: true },
    model: { type: String },
    asset_id: { type: String },
  },
  { _id: false }
);

const ActuatorSchema = new Schema(
  {
    type: { type: String, required: true },
    channel: { type: String },
    state: { type: String, enum: ['ON', 'OFF', 'AUTO'], default: 'AUTO' },
  },
  { _id: false }
);

const SetpointsSchema = new Schema(
  {
    t_c: { type: Number, default: 22 },
    rh: { type: Number, default: 55 },
    lux: { type: Number, default: 200 },
  },
  { _id: false }
);

const ThresholdSchema = new Schema(
  {
    mq135_ppm: { type: Number, default: 200 },
    t_c_high: { type: Number, default: 30 },
    t_c_low: { type: Number, default: 18 },
    rh_high: { type: Number, default: 70 },
    rh_low: { type: Number, default: 40 },
  },
  { _id: false }
);

const PPSchema = new Schema({
  pp_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  building_id: { type: String, required: true },
  zone_id: { type: String, required: true },
  mode: { type: String, enum: ['PONDOIR', 'POUSSINIERE'], required: true },
  state: { type: String, enum: ['DISPO', 'OCCUPE', 'MAINT'], default: 'DISPO' },
  lot_id: { type: String },
  sensors: { type: [SensorSchema], default: [] },
  actuators: { type: [ActuatorSchema], default: [] },
  setpoints: { type: SetpointsSchema, default: () => ({}) },
  thresholds: { type: ThresholdSchema, default: () => ({}) },
  lastCalibrationAt: { type: Date },
});

PPSchema.index({ site_id: 1, pp_id: 1 });

auditTrailPlugin(PPSchema);
dynamicFieldsPlugin('pp')(PPSchema);

export const PPModel = mongoose.models.PP || mongoose.model('PP', PPSchema);
