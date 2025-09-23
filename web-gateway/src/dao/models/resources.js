import mongoose from 'mongoose';
import { auditTrailPlugin, dynamicFieldsPlugin } from './common.js';

const { Schema } = mongoose;

const MovementSchema = new Schema(
  {
    ts: { type: Date, default: Date.now },
    type: { type: String, enum: ['IN', 'OUT', 'ADJUST'], required: true },
    quantity: { type: Number, required: true },
    unit: { type: String, required: true },
    reference: { type: String },
  },
  { _id: false }
);

const ResourceSchema = new Schema({
  resource_id: { type: String, required: true, unique: true },
  site_id: { type: String, required: true },
  category: {
    type: String,
    enum: ['FEED', 'WATER', 'ENERGY', 'MEDICAL', 'SUPPLY'],
    required: true,
  },
  name: { type: String, required: true },
  stock_level: { type: Number, default: 0 },
  unit: { type: String, default: 'kg' },
  reorder_point: { type: Number, default: 0 },
  movements: { type: [MovementSchema], default: [] },
});

ResourceSchema.index({ site_id: 1, category: 1 });

auditTrailPlugin(ResourceSchema);
dynamicFieldsPlugin('resource')(ResourceSchema);

export const ResourceModel =
  mongoose.models.Resource || mongoose.model('Resource', ResourceSchema);
