import { Router } from 'express';
import { getKpiSnapshot } from '../../dao/services/kpi.service.js';
import { permissions } from '../../rbac/permissions.js';
import { rbac } from '../../auth/rbac.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';

const router = Router();

router.get('/', asyncHandler(async (req, res) => {
  if (!rbac.hasPermission(req.user, permissions.VIEW_KPI)) {
    throw new ServiceError('Accès refusé', { status: 403, code: 'FORBIDDEN' });
  }
  const { scope = 'site', id = 'SITE:KIN-GOLIATH' } = req.query;
  const scopeFilter = { [scope]: id };
  const data = await getKpiSnapshot({ scope: scopeFilter });
  res.json(data);
}));

export default router;
