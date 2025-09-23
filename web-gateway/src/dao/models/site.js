import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const CameraSchema = new Schema(
  {
    camera_id: { type: String, required: true },
    label: { type: String, required: true },
    rtsp_url: { type: String, required: true },
    onvif: { type: String },
  },
  { _id: false }
);

const SiteSchema = new Schema({
  site_id: { type: String, required: true, unique: true },
  name: { type: String, required: true },
  timezone: { type: String, default: 'Africa/Kinshasa' },
  gps: {
    lat: { type: Number, required: true },
    lng: { type: Number, required: true },
  },
  cameras: { type: [CameraSchema], default: [] },
  assets: { type: [String], default: [] },
});

SiteSchema.index({ site_id: 1 });

auditTrailPlugin(SiteSchema);
dynamicFieldsPlugin('site')(SiteSchema);

export const SiteModel = mongoose.models.Site || mongoose.model('Site', SiteSchema);
