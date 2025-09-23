import mongoose from 'mongoose';
import { auditTrailPlugin } from './common.js';

const { Schema } = mongoose;

const FieldDefSchema = new Schema(
  {
    scope: { type: String, required: true },
    key: { type: String, required: true },
    label: { type: String, required: true },
    type: { type: String, required: true },
    unit: { type: String },
    min: { type: Number },
    max: { type: Number },
    regex: { type: String },
    enum_values: { type: [String], default: [] },
    required: { type: Boolean, default: false },
    ui_hint: { type: String },
    version: { type: Number, default: 1 },
    effective_from: { type: Date, default: Date.now },
    effective_to: { type: Date },
  },
  { toJSON: { virtuals: true }, toObject: { virtuals: true } }
);

FieldDefSchema.virtual('effective').get(function computeEffective() {
  const now = new Date();
  if (this.effective_from && this.effective_from > now) return false;
  if (this.effective_to && this.effective_to < now) return false;
  return true;
});

FieldDefSchema.index({ scope: 1, key: 1, version: -1 }, { unique: true });

auditTrailPlugin(FieldDefSchema);

export const FieldDefModel = mongoose.models.FieldDef || mongoose.model('FieldDef', FieldDefSchema);
