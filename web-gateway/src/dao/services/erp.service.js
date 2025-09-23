import dayjs from 'dayjs';
import { LotModel } from '../models/lot.js';
import { IncubationModel } from '../models/incubation.js';
import { ResourceModel } from '../models/resources.js';
import { PPModel } from '../models/pp.js';
import { ServiceError } from '../../utils/errors.js';

const ensureCapacity = async (ppIds = []) => {
  const pps = await PPModel.find({ pp_id: { $in: ppIds } }).lean();
  if (pps.some((pp) => pp.state === 'OCCUPE')) {
    throw new ServiceError('PP occupé, impossible d\'assigner le lot', {
      status: 409,
      code: 'PP_OCCUPE',
    });
  }
};

export const listLots = async (filter = {}) => {
  const query = LotModel.find(filter).sort({ createdAt: -1 });
  return query.lean();
};

export const createLot = async (input, actor) => {
  await ensureCapacity(input.pp_ids);
  const lot = await LotModel.create({
    ...input,
    timeline: [
      { ts: new Date(), type: 'lot.created', payload: { actor } },
    ],
  });
  if (input.pp_ids?.length) {
    await PPModel.updateMany(
      { pp_id: { $in: input.pp_ids } },
      { $set: { state: 'OCCUPE', lot_id: lot.lot_id } }
    );
  }
  return lot.toObject();
};

export const updateLot = async (lotId, patch, actor) => {
  const lot = await LotModel.findOneAndUpdate(
    { lot_id: lotId },
    {
      $set: patch,
      $push: { timeline: { ts: new Date(), type: 'lot.updated', payload: { actor, patch } } },
    },
    { new: true }
  ).lean();
  return lot;
};

export const closeLot = async (lotId, closure = {}, actor) => {
  const lot = await LotModel.findOneAndUpdate(
    { lot_id: lotId },
    {
      $set: { status: 'TERMINE', ...closure },
      $push: {
        timeline: {
          ts: new Date(),
          type: 'lot.closed',
          payload: { actor, closure },
        },
      },
    },
    { new: true }
  );
  if (lot?.pp_ids?.length) {
    await PPModel.updateMany(
      { pp_id: { $in: lot.pp_ids } },
      { $set: { state: 'DISPO', lot_id: null } }
    );
  }
  return lot?.toObject();
};

export const listIncubations = async (filter = {}) =>
  IncubationModel.find(filter).sort({ start_date: -1 }).lean();

export const recordHatch = async ({ incubationId, hatched, actor }) => {
  const incubation = await IncubationModel.findOne({ incubation_id: incubationId });
  if (!incubation) {
    throw new ServiceError('Incubation inconnue', { status: 404, code: 'INCUBATION_NOT_FOUND' });
  }
  const hatchRate = hatched && incubation.eggs_loaded ? (hatched / incubation.eggs_loaded) * 100 : 0;
  incubation.hatch_date = new Date();
  incubation.hatch_rate = Number(hatchRate.toFixed(2));
  incubation.status = 'TERMINE';
  incubation.audit_log.push({ actor, action: 'hatch.record', meta: { hatched } });
  await incubation.save();
  const lotId = `LOT-${dayjs().format('YYYYMMDD')}-${Math.random().toString(16).slice(2, 6)}`;
  const lot = await LotModel.create({
    lot_id: lotId,
    site_id: incubation.site_id,
    building_id: 'BLDG:A',
    zone_id: 'A-Z1',
    nb_initial: hatched,
    nb_current: hatched,
    status: 'EN_COURS',
    incubations: [incubationId],
    timeline: [
      { ts: new Date(), type: 'lot.created', payload: { actor, source: incubationId } },
    ],
  });
  return { incubation: incubation.toObject(), lot: lot.toObject() };
};

export const adjustResource = async ({ resourceId, quantity, unit, type, reference }) => {
  const movement = { ts: new Date(), type, quantity, unit, reference };
  const resource = await ResourceModel.findOneAndUpdate(
    { resource_id: resourceId },
    {
      $inc: { stock_level: type === 'OUT' ? -quantity : quantity },
      $push: { movements: movement },
    },
    { new: true }
  );
  return resource?.toObject();
};
