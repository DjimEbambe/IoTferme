import { expect } from 'chai';
import sinon from 'sinon';
import * as lotModule from '../src/dao/models/lot.js';
import * as zoneModule from '../src/dao/models/zone.js';
import * as resourceModule from '../src/dao/models/resources.js';
import { getKpiSnapshot } from '../src/dao/services/kpi.service.js';
import * as influxQueries from '../src/influx/queries.js';

const sandbox = sinon.createSandbox();

const stubQuery = (data) => ({
  lean: sandbox.stub().resolves(data),
  sort: sandbox.stub().returnsThis(),
});

describe('KPI service', () => {
  beforeEach(() => {
    sandbox.stub(lotModule, 'LotModel').value({
      find: sandbox.stub().returns(stubQuery([
        {
          lot_id: 'L1',
          nb_current: 390,
          nb_initial: 400,
          kpis: { fcr: 1.6, gmq: 60 },
          status: 'EN_COURS',
        },
      ])),
    });
    sandbox.stub(zoneModule, 'ZoneModel').value({
      find: sandbox.stub().returns(stubQuery([
        { zone_id: 'A-Z1', surface_m2: 35, occupancy: 300, capacity: 400 },
      ])),
    });
    sandbox.stub(resourceModule, 'ResourceModel').value({
      find: sandbox.stub().returns(stubQuery([
        { resource_id: 'R1', category: 'FEED', stock_level: 100, unit: 'kg', reorder_point: 50 },
      ])),
    });
    sandbox.stub(influxQueries, 'fetchRangeAggregates').resolves([
      { value: 10 },
      { value: 12 },
    ]);
  });

  afterEach(() => sandbox.restore());

  it('computes KPI snapshot', async () => {
    const kpi = await getKpiSnapshot({ scope: { site: 'SITE:KIN-GOLIATH' } });
    expect(kpi.kpis.lots_active).to.equal(1);
    expect(kpi.kpis.mortality).to.be.closeTo(2.5, 0.1);
    expect(kpi.resources).to.have.lengthOf(1);
  });
});
