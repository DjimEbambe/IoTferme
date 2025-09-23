import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const AssetSchema = new Schema({
  asset_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  type: {
    type: String,
    enum: ['POMPE', 'CUVE', 'INCUBATEUR', 'ENERGIE', 'CAMERA', 'SENSOR', 'ACTUATOR', 'AUTRE'],
    required: true,
  },
  label: { type: String, required: true },
  location: { type: String },
  specs: { type: Schema.Types.Mixed },
  status: { type: String, enum: ['OK', 'WARN', 'ALARM', 'OFFLINE'], default: 'OK' },
});

AssetSchema.index({ site_id: 1, type: 1 });

auditTrailPlugin(AssetSchema);
dynamicFieldsPlugin('asset')(AssetSchema);

export const AssetModel = mongoose.models.Asset || mongoose.model('Asset', AssetSchema);
