import { Router } from 'express';
import Ajv from 'ajv';
import addFormats from 'ajv-formats';
import { publishCommand } from '../../mqtt/publisher.js';
import { permissions } from '../../rbac/permissions.js';
import { rbac } from '../../auth/rbac.js';
import { PPModel } from '../../dao/models/pp.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';

const router = Router();
const ajv = new Ajv({ coerceTypes: true, strict: false, allowUnionTypes: true, allErrors: true });
addFormats(ajv);

const schema = {
  type: 'object',
  required: ['site', 'device', 'asset_id'],
  properties: {
    site: { type: 'string' },
    device: { type: 'string' },
    asset_id: { type: 'string' },
    relays: { type: 'object', additionalProperties: { enum: ['ON', 'OFF'] } },
    setpoints: {
      type: 'object',
      additionalProperties: { type: ['number', 'string'] },
    },
    sequence: { type: 'array', items: { type: 'object' } },
  },
  additionalProperties: false,
};

const validate = ajv.compile(schema);

router.post('/', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.ISSUE_COMMAND)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }

  const body = req.body;
  const valid = validate(body);
  if (!valid) {
    throw new ServiceError('Payload invalide', {
      status: 400,
      code: 'VALIDATION_FAILED',
      details: validate.errors,
    });
  }

  let ack;
  try {
    ack = await publishCommand(body);
  } catch (error) {
    throw new ServiceError(error.message || 'ACK indisponible', {
      status: 504,
      code: 'CMD_ACK_TIMEOUT',
    });
  }

  const actor = req.user?.displayName || req.user?.email || req.user?.phone || req.user?.id;
  await PPModel.updateOne(
    { pp_id: body.asset_id },
    {
      $push: {
        audit_log: {
          actor,
          action: 'command.sent',
          meta: { command: body, ack },
        },
      },
    }
  );

  res.json({ ok: true, ack });
}));

export default router;
