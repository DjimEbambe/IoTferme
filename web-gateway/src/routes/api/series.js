import { Router } from 'express';
import { fetchAssetMetricSeries } from '../../influx/queries.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';

const router = Router();

router.get('/', asyncHandler(async (req, res) => {
  const { metric, asset_id: assetId, range = '24h', agg = 'mean' } = req.query;
  if (!metric || !assetId) {
    throw new ServiceError('metric et asset_id requis', {
      status: 400,
      code: 'VALIDATION_FAILED',
    });
  }
  const series = await fetchAssetMetricSeries({ metric, assetId, range, aggregate: agg });
  res.json({ metric, assetId, series });
}));

export default router;
