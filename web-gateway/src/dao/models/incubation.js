import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const IncubationSchema = new Schema({
  incubation_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  incubator_id: { type: String, required: true },
  eggs_loaded: { type: Number, required: true },
  start_date: { type: Date, required: true },
  expected_hatch_date: { type: Date, required: true },
  hatch_date: { type: Date },
  hatch_rate: { type: Number },
  lot_id: { type: String },
  status: { type: String, enum: ['EN_COURS', 'TERMINE', 'ARCHIVE'], default: 'EN_COURS' },
  notes: { type: String },
});

IncubationSchema.index({ site_id: 1, status: 1 });

auditTrailPlugin(IncubationSchema);
dynamicFieldsPlugin('incubation')(IncubationSchema);

export const IncubationModel =
  mongoose.models.Incubation || mongoose.model('Incubation', IncubationSchema);
