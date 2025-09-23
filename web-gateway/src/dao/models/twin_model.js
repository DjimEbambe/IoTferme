import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const TwinAssetSchema = new Schema(
  {
    asset_id: { type: String, required: true },
    type: { type: String, required: true },
    position: {
      x: { type: Number, default: 0 },
      y: { type: Number, default: 0 },
      z: { type: Number, default: 0 },
    },
    rotation: {
      x: { type: Number, default: 0 },
      y: { type: Number, default: 0 },
      z: { type: Number, default: 0 },
    },
    scale: { type: Number, default: 1 },
    metadata: { type: Map, of: String },
  },
  { _id: false }
);

const TwinModelSchema = new Schema({
  site_id: { type: String, required: true },
  layout_version: { type: Number, default: 1 },
  assets: { type: [TwinAssetSchema], default: [] },
  heatmap_overrides: {
    temperature: { type: Number, default: 0 },
    humidity: { type: Number, default: 0 },
    air_quality: { type: Number, default: 0 },
  },
  editor_state: { type: Schema.Types.Mixed },
});

TwinModelSchema.index({ site_id: 1 }, { unique: true });

auditTrailPlugin(TwinModelSchema);
dynamicFieldsPlugin('twin')(TwinModelSchema);

export const TwinModel = mongoose.models.TwinModel || mongoose.model('TwinModel', TwinModelSchema);
