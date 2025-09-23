import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const ZoneSchema = new Schema({
  zone_id: { type: String, required: true, unique: true },
  building_id: { type: String, required: true },
  site_id: { type: String, required: true },
  surface_m2: { type: Number, required: true },
  capacity: { type: Number, required: true },
  occupancy: { type: Number, default: 0 },
  kpis: {
    density: { type: Number, default: 0 },
    fcr_local: { type: Number },
    gmq: { type: Number },
  },
  devices: { type: [String], default: [] },
  alerts: { type: [String], default: [] },
});

ZoneSchema.index({ building_id: 1, zone_id: 1 });

ZoneSchema.virtual('density_current').get(function densityCurrent() {
  if (!this.surface_m2 || !this.occupancy) return 0;
  return this.occupancy / this.surface_m2;
});

auditTrailPlugin(ZoneSchema);
dynamicFieldsPlugin('zone')(ZoneSchema);

export const ZoneModel = mongoose.models.Zone || mongoose.model('Zone', ZoneSchema);
