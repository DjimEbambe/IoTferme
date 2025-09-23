import { Router } from 'express';
import { permissions } from '../../rbac/permissions.js';
import { rbac } from '../../auth/rbac.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';
import {
  listLots,
  createLot,
  updateLot,
  closeLot,
  listIncubations,
  recordHatch,
  adjustResource,
} from '../../dao/services/erp.service.js';

const router = Router();

router.get('/lots', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.VIEW_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const lots = await listLots({ site_id: req.user?.scopes?.site?.[0] });
  res.json({ items: lots });
}));

router.post('/lots', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const actor = req.user?.displayName || req.user?.email || req.user?.phone || req.user?.id;
  const lot = await createLot(req.body, actor);
  res.status(201).json(lot);
}));

router.patch('/lots/:lotId', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const actor = req.user?.displayName || req.user?.email || req.user?.phone || req.user?.id;
  const lot = await updateLot(req.params.lotId, req.body, actor);
  if (!lot) {
    throw new ServiceError('Lot introuvable', { status: 404, code: 'LOT_NOT_FOUND' });
  }
  res.json(lot);
}));

router.post('/lots/:lotId/close', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const actor = req.user?.displayName || req.user?.email || req.user?.phone || req.user?.id;
  const lot = await closeLot(req.params.lotId, req.body, actor);
  if (!lot) {
    throw new ServiceError('Lot introuvable', { status: 404, code: 'LOT_NOT_FOUND' });
  }
  res.json(lot);
}));

router.get('/incubations', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.VIEW_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const incubations = await listIncubations({ site_id: req.user?.scopes?.site?.[0] });
  res.json({ items: incubations });
}));

router.post('/incubations/:id/hatch', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const hatched = Number(req.body.hatched);
  if (Number.isNaN(hatched)) {
    throw new ServiceError('Valeur hatched invalide', { status: 400, code: 'VALIDATION_FAILED' });
  }
  const actor = req.user?.displayName || req.user?.email || req.user?.phone || req.user?.id;
  const result = await recordHatch({ incubationId: req.params.id, hatched, actor });
  res.json(result);
}));

router.post('/resources/:id/adjust', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.MANAGE_ERP)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const quantity = Number(req.body.quantity);
  if (Number.isNaN(quantity)) {
    throw new ServiceError('Quantité invalide', { status: 400, code: 'VALIDATION_FAILED' });
  }
  const result = await adjustResource({
    resourceId: req.params.id,
    quantity,
    unit: req.body.unit,
    type: req.body.type,
    reference: req.body.reference,
  });
  if (!result) {
    throw new ServiceError('Ressource introuvable', { status: 404, code: 'RESOURCE_NOT_FOUND' });
  }
  res.json(result);
}));

export default router;
