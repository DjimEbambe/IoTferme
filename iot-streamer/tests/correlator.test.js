import { expect } from 'chai';
import sinon from 'sinon';
import { trackCommand, resolveAck, clearPending } from '../src/correlator.js';
import * as redisBus from '../src/redis_bus.js';
import { config } from '../src/config.js';

const sandbox = sinon.createSandbox();

describe('Correlator', () => {
  beforeEach(() => {
    sandbox.stub(redisBus, 'publishAck').resolves();
    sandbox.stub(redisBus, 'publishIncident').resolves();
    config.ackTimeoutMs = 50;
    config.ackRetryLimit = 0;
  });

  afterEach(() => {
    clearPending();
    sandbox.restore();
  });

  it('resolves ACK and publishes latency', async () => {
    trackCommand({ correlation_id: 'abc', site: 'SITE:KIN-GOLIATH', asset_id: 'A-01' });
    await resolveAck({ correlation_id: 'abc', asset_id: 'A-01', ok: true });
    expect(redisBus.publishAck.calledOnce).to.be.true;
    const payload = redisBus.publishAck.firstCall.args[0];
    expect(payload.ok).to.be.true;
  });

  it('handles ACK without pending command', async () => {
    await resolveAck({ correlation_id: 'ghost', asset_id: 'A', ok: false });
    expect(redisBus.publishAck.calledOnce).to.be.true;
    expect(redisBus.publishIncident.called).to.be.false;
  });
});
