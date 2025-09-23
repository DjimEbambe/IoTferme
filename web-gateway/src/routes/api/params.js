import { Router } from 'express';
import Ajv from 'ajv';
import { FieldDefModel } from '../../dao/models/field_def.js';
import { permissions } from '../../rbac/permissions.js';
import { rbac } from '../../auth/rbac.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';

const router = Router();
const ajv = new Ajv({ allErrors: true, strict: false });

const payloadSchema = {
  type: 'object',
  required: ['scope', 'key'],
  properties: {
    scope: { type: 'string' },
    key: { type: 'string' },
    label: { type: 'string' },
    type: { type: 'string' },
    unit: { type: 'string' },
    min: { type: 'number' },
    max: { type: 'number' },
    regex: { type: 'string' },
    enum_values: { type: 'array', items: { type: 'string' } },
    required: { type: 'boolean' },
    ui_hint: { type: 'string' },
  },
  additionalProperties: false,
};

const validate = ajv.compile(payloadSchema);

router.get('/', asyncHandler(async (req, res) => {
  const { scope } = req.query;
  const filter = scope ? { scope } : {};
  const defs = await FieldDefModel.find(filter).sort({ scope: 1, key: 1 }).lean();
  res.json({ items: defs });
}));

router.post('/', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const data = req.body;
  if (!validate(data)) {
    throw new ServiceError('Payload invalide', {
      status: 400,
      code: 'VALIDATION_FAILED',
      details: validate.errors,
    });
  }
  const version = (await FieldDefModel.countDocuments({ scope: data.scope, key: data.key })) + 1;
  try {
    const def = await FieldDefModel.create({ ...data, version });
    res.status(201).json(def.toObject());
  } catch (error) {
    if (error?.code === 11000) {
      throw new ServiceError('Champ déjà existant', { status: 409, code: 'FIELDDEF_DUPLICATE' });
    }
    throw error;
  }
}));

export default router;
