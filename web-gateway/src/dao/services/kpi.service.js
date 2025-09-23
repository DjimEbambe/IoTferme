import dayjs from 'dayjs';
import { LotModel } from '../models/lot.js';
import { ZoneModel } from '../models/zone.js';
import { ResourceModel } from '../models/resources.js';
import { fetchRangeAggregates } from '../../influx/queries.js';

const defaultRange = () => ({ start: dayjs().subtract(24, 'hour').toDate(), stop: new Date() });

const scopeFilter = (scope) => {
  if (!scope) return {};
  if (scope.site) return { site_id: scope.site };
  if (scope.building) return { building_id: scope.building };
  if (scope.zone) return { zone_id: scope.zone };
  return {};
};

export const getKpiSnapshot = async ({ scope = { site: 'SITE:KIN-GOLIATH' }, range } = {}) => {
  const lots = await LotModel.find({ ...scopeFilter(scope), status: 'EN_COURS' }).lean();
  const zones = await ZoneModel.find(scopeFilter(scope)).lean();
  const resources = await ResourceModel.find({ site_id: scope.site || 'SITE:KIN-GOLIATH' }).lean();

  const lotCount = lots.length;
  const totalBirds = lots.reduce((acc, lot) => acc + (lot.nb_current || 0), 0);
  const totalInitial = lots.reduce((acc, lot) => acc + (lot.nb_initial || 0), 0);
  const avgFcr = lotCount ? lots.reduce((acc, lot) => acc + (lot.kpis?.fcr || 0), 0) / lotCount : 0;
  const avgGmq = lotCount ? lots.reduce((acc, lot) => acc + (lot.kpis?.gmq || 0), 0) / lotCount : 0;
  const mortality = totalInitial ? ((totalInitial - totalBirds) / totalInitial) * 100 : 0;

  const zoneAlerts = zones.filter((z) => z.occupancy > z.capacity).map((z) => z.zone_id);
  const densityAvg = zones.length
    ? zones.reduce((acc, zone) => acc + (zone.occupancy / Math.max(zone.surface_m2, 1)), 0) / zones.length
    : 0;

  const resourceSummary = resources.map((resource) => ({
    resource_id: resource.resource_id,
    category: resource.category,
    stock_level: resource.stock_level,
    unit: resource.unit,
    reorder_point: resource.reorder_point,
    below_reorder: resource.stock_level <= resource.reorder_point,
  }));

  const { start, stop } = range || defaultRange();
  const [waterSeries, energySeries] = await Promise.all([
    fetchRangeAggregates({
      siteId: scope.site || 'SITE:KIN-GOLIATH',
      measurement: 'water',
      field: 'volume_l',
      start,
      stop,
      aggregate: 'sum',
    }),
    fetchRangeAggregates({
      siteId: scope.site || 'SITE:KIN-GOLIATH',
      measurement: 'power',
      field: 'kwh',
      start,
      stop,
      aggregate: 'sum',
    }),
  ]);

  const waterTotal = waterSeries.reduce((acc, point) => acc + point.value, 0);
  const energyTotal = energySeries.reduce((acc, point) => acc + point.value, 0);

  return {
    range: { start, stop },
    lots: lots.slice(0, 5).map((lot) => ({ lot_id: lot.lot_id, status: lot.status, kpis: lot.kpis, nb_current: lot.nb_current })),
    kpis: {
      lots_active: lotCount,
      birds_current: totalBirds,
      fcr: Number(avgFcr.toFixed(2)),
      gmq: Number(avgGmq.toFixed(2)),
      mortality: Number(mortality.toFixed(2)),
      water_per_kg: totalBirds ? Number((waterTotal / totalBirds).toFixed(2)) : 0,
      kwh_per_kg: totalBirds ? Number((energyTotal / totalBirds).toFixed(2)) : 0,
      density_avg: Number(densityAvg.toFixed(2)),
    },
    alerts: { density: zoneAlerts },
    resources: resourceSummary,
  };
};

export const getLotTimeline = async (lotId) => {
  const lot = await LotModel.findOne({ lot_id: lotId }).lean();
  if (!lot) return [];
  return lot.timeline || [];
};
