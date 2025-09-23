import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const BuildingSchema = new Schema({
  building_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  name: { type: String, required: true },
  surface_m2: { type: Number, required: true },
  nb_zones: { type: Number, required: true },
  usage: { type: String, enum: ['production', 'maturation', 'incubation', 'maintenance'], required: true },
  cameras: { type: [String], default: [] },
  pondobrood_count: { type: Number, default: 8 },
});

BuildingSchema.index({ site_id: 1, building_id: 1 });

auditTrailPlugin(BuildingSchema);
dynamicFieldsPlugin('building')(BuildingSchema);

export const BuildingModel = mongoose.models.Building || mongoose.model('Building', BuildingSchema);
