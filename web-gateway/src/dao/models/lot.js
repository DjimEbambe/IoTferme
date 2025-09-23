import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const TimelineEventSchema = new Schema(
  {
    ts: { type: Date, default: Date.now },
    type: { type: String, required: true },
    payload: { type: Schema.Types.Mixed },
  },
  { _id: false }
);

const LotSchema = new Schema({
  lot_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  building_id: { type: String },
  zone_id: { type: String },
  target_weight_kg: { type: Number },
  age_days: { type: Number, default: 0 },
  nb_initial: { type: Number, required: true },
  nb_current: { type: Number, required: true },
  weight_avg_g: { type: Number },
  kpis: {
    gmq: { type: Number },
    fcr: { type: Number },
    mortality: { type: Number },
    water_per_kg: { type: Number },
    kwh_per_kg: { type: Number },
  },
  status: {
    type: String,
    enum: ['EN_COURS', 'TERMINE', 'ARCHIVE'],
    default: 'EN_COURS',
  },
  incubations: { type: [String], default: [] },
  pp_ids: { type: [String], default: [] },
  timeline: { type: [TimelineEventSchema], default: [] },
});

LotSchema.index({ site_id: 1, status: 1 });

auditTrailPlugin(LotSchema);
dynamicFieldsPlugin('lot')(LotSchema);

export const LotModel = mongoose.models.Lot || mongoose.model('Lot', LotSchema);
