import { TwinModel } from '../models/twin_model.js';
import { PPModel } from '../models/pp.js';
import { ServiceError } from '../../utils/errors.js';

export const getTwinState = async (siteId = 'SITE:KIN-GOLIATH') => {
  const twin = await TwinModel.findOne({ site_id: siteId }).lean();
  return twin || { site_id: siteId, assets: [], layout_version: 0 };
};

export const saveLayout = async (siteId, { assets, editor_state }, actor) => {
  const result = await TwinModel.findOneAndUpdate(
    { site_id: siteId },
    {
      $set: {
        assets,
        editor_state,
      },
      $inc: { layout_version: 1 },
      $push: {
        audit_log: {
          actor,
          action: 'twin.layout.update',
          meta: { count: assets?.length || 0 },
        },
      },
    },
    { new: true, upsert: true }
  );
  return result.toObject();
};

export const calibrateSensor = async ({ ppId, offsets }) => {
  const pp = await PPModel.findOneAndUpdate(
    { pp_id: ppId },
    {
      $set: offsets,
      $push: {
        audit_log: {
          actor: 'system',
          action: 'sensor.calibration',
          meta: offsets,
        },
      },
    },
    { new: true }
  );
  if (!pp) {
    throw new ServiceError('PP introuvable', { status: 404, code: 'PP_NOT_FOUND' });
  }
  return pp.toObject();
};
