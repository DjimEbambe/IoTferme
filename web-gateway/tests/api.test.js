import express from 'express';
import request from 'supertest';
import { expect } from 'chai';
import sinon from 'sinon';
import router from '../src/routes/api/cmd.js';
import * as publisher from '../src/mqtt/publisher.js';
import * as rbacModule from '../src/auth/rbac.js';
import * as ppModule from '../src/dao/models/pp.js';
import { formatErrorResponse } from '../src/utils/errors.js';

const sandbox = sinon.createSandbox();

describe('Command API', () => {
  let app;

  beforeEach(() => {
    sandbox.stub(rbacModule, 'rbac').value({
      hasPermission: sandbox.stub().returns(true),
    });
    sandbox.stub(publisher, 'publishCommand').resolves({ ok: true });
    sandbox.stub(ppModule, 'PPModel').value({
      updateOne: sandbox.stub().resolves(),
    });
    app = express();
    app.use(express.json());
    app.use((req, res, next) => {
      req.user = { email: 'test@farmstack.local', roles: ['admin'] };
      req.id = 'test-request-id';
      next();
    });
    app.use('/api/cmd', router);
    app.use((err, req, res, next) => {
      const { status, payload } = formatErrorResponse(err, req);
      res.status(status).json(payload);
    });
  });

  afterEach(() => sandbox.restore());

  it('rejects invalid payload', async () => {
    const res = await request(app).post('/api/cmd').send({});
    expect(res.status).to.equal(400);
  });

  it('publishes valid command', async () => {
    const payload = {
      site: 'SITE:KIN-GOLIATH',
      device: 'BLDG:A-PP-01',
      asset_id: 'BLDG:A-PP-01',
      relays: { lamp: 'ON' },
    };
    const res = await request(app).post('/api/cmd').send(payload);
    expect(res.status).to.equal(200);
    expect(publisher.publishCommand.calledOnce).to.be.true;
  });
});
