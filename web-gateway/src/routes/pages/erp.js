import { Router } from 'express';
import { listLots, listIncubations } from '../../dao/services/erp.service.js';
import { asyncHandler } from '../../utils/errors.js';

const router = Router();

router.get('/', asyncHandler(async (req, res) => {
  const scope = { site_id: req.user?.scopes?.site?.[0] };
  const [lots, incubations] = await Promise.all([
    listLots(scope),
    listIncubations(scope),
  ]);
  res.render('erp/index', {
    title: 'ERP',
    nonce: res.locals.cspNonce,
    csrfToken: res.locals.csrfToken,
    lots,
    incubations,
  });
}));

export default router;
