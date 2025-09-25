import { Router } from 'express';
import { getTwinState } from '../../dao/services/twin.service.js';
import { asyncHandler } from '../../utils/errors.js';
import { isProd } from '../../config/env.js';

const router = Router();

router.get('/', asyncHandler(async (req, res) => {
  const siteId = req.user?.scopes?.site?.[0] || 'SITE:KIN-GOLIATH';
  const twin = await getTwinState(siteId);
  res.render('twins', {
    title: 'Jumeau Numérique',
    nonce: res.locals.cspNonce,
    csrfToken: res.locals.csrfToken,
    twin,
    modelRoot: isProd ? '/dist/twins/' : '/twins/',
  });
}));

export default router;
