import mongoose from 'mongoose';

const { Schema } = mongoose;

const DynamicFieldValueSchema = new Schema(
  {
    v: { type: Schema.Types.Mixed, required: true },
    t: { type: String, required: true },
    unit: { type: String },
    updatedAt: { type: Date, default: Date.now },
  },
  { _id: false }
);

const AuditEventSchema = new Schema(
  {
    actor: { type: String },
    action: { type: String, required: true },
    at: { type: Date, default: Date.now },
    meta: { type: Schema.Types.Mixed },
  },
  { _id: false }
);

export const dynamicFieldsPlugin = (scope) => (schema) => {
  schema.add({
    dyn_fields: {
      type: Map,
      of: DynamicFieldValueSchema,
      default: undefined,
    },
  });

  schema.pre('validate', async function validateDynamicFields() {
    if (!this.dyn_fields || this.dyn_fields.size === 0) return;
    const FieldDefModel = mongoose.models.FieldDef || (await import('./field_def.js')).FieldDefModel;
    const defs = await FieldDefModel.find({ scope }).lean();
    const now = Date.now();
    const defMap = new Map(
      defs
        .filter((def) => {
          const fromOk = !def.effective_from || new Date(def.effective_from).getTime() <= now;
          const toOk = !def.effective_to || new Date(def.effective_to).getTime() >= now;
          return fromOk && toOk;
        })
        .map((def) => [def.key, def])
    );
    for (const [key, value] of this.dyn_fields.entries()) {
      const def = defMap.get(key);
      if (!def) {
        throw new Error(`Dynamic field ${key} not allowed for scope ${scope}`);
      }
      if (def.required && (value?.v === undefined || value?.v === null || value?.v === '')) {
        throw new Error(`Dynamic field ${key} is required`);
      }
      if (value?.t && def.type && value.t !== def.type) {
        throw new Error(`Dynamic field ${key} must be of type ${def.type}`);
      }
      if (def.regex) {
        const re = new RegExp(def.regex);
        if (!re.test(String(value.v))) {
          throw new Error(`Dynamic field ${key} does not match pattern`);
        }
      }
      if (def.min !== undefined && typeof value.v === 'number' && value.v < def.min) {
        throw new Error(`Dynamic field ${key} below min ${def.min}`);
      }
      if (def.max !== undefined && typeof value.v === 'number' && value.v > def.max) {
        throw new Error(`Dynamic field ${key} above max ${def.max}`);
      }
      if (def.enum_values?.length && !def.enum_values.includes(value.v)) {
        throw new Error(`Dynamic field ${key} not in enum`);
      }
      value.unit = value.unit || def.unit;
    }
  });
};

export const auditTrailPlugin = (schema) => {
  schema.add({
    createdAt: { type: Date, default: Date.now },
    updatedAt: { type: Date, default: Date.now },
    createdBy: { type: String },
    updatedBy: { type: String },
    audit_log: { type: [AuditEventSchema], default: [] },
  });

  schema.pre('save', function updateTimestamp(next) {
    this.updatedAt = new Date();
    next();
  });
};

const UserSchema = new Schema(
  {
    email: { type: String, unique: true, lowercase: true, sparse: true },
    phone: {
      type: String,
      unique: true,
      sparse: true,
      set: (value) => (value ? value.replace(/\D/g, '') : value),
    },
    passwordHash: { type: String, required: true },
    displayName: { type: String },
    roles: { type: [String], default: ['visiteur'] },
    scopes: { type: Map, of: [String], default: {} },
    active: { type: Boolean, default: true },
    lastLoginAt: { type: Date },
  },
  { timestamps: true }
);

UserSchema.index({ email: 1 }, { unique: true, sparse: true });
UserSchema.index({ phone: 1 }, { unique: true, sparse: true });

UserSchema.pre('validate', function ensureContact(next) {
  if (!this.email && !this.phone) {
    next(new Error('Un contact email ou téléphone est requis'));
    return;
  }
  next();
});

export const UserModel = mongoose.models.User || mongoose.model('User', UserSchema);

export const createConnection = async (mongoUri, user, password) => {
  const connectionOptions = {
    maxPoolSize: 10,
    serverSelectionTimeoutMS: 5000,
    appName: 'farmstack-web-gateway',
  };
  if (user && password) {
    connectionOptions.auth = { username: user, password };
  }
  await mongoose.connect(mongoUri, connectionOptions);
  return mongoose.connection;
};
