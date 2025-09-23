import { Router } from 'express';
import dayjs from 'dayjs';
import { getKpiSnapshot, getLotTimeline } from '../../dao/services/kpi.service.js';
import { permissions } from '../../rbac/permissions.js';
import { rbac } from '../../auth/rbac.js';
import { asyncHandler } from '../../utils/errors.js';

const router = Router();

router.get('/', asyncHandler(async (req, res) => {
  const scope = { site: req.user?.scopes?.site?.[0] || 'SITE:KIN-GOLIATH' };
  const kpi = await getKpiSnapshot({ scope });
  const lotTimeline = await getLotTimeline(kpi?.lots?.[0]?.lot_id);

  res.render('dashboard', {
    title: 'Tableau de bord',
    nonce: res.locals.cspNonce,
    csrfToken: res.locals.csrfToken,
    today: dayjs().format('DD MMM YYYY'),
    kpi,
    lotTimeline,
    canIssueCmd: rbac.hasPermission(req.user, permissions.ISSUE_COMMAND),
  });
}));

export default router;
