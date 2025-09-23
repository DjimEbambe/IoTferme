import '../models/field_def.js';
import { createConnection } from '../models/common.js';
import { seedInitialData } from './init.seed.js';
import { env } from '../../config/env.js';
import { logger } from '../../logger.js';

const run = async () => {
  await createConnection(env.mongoUri, env.mongoUser, env.mongoPassword);
  await seedInitialData();
  logger.info('Seed terminé');
  process.exit(0);
};

run().catch((err) => {
  console.error(err);
  process.exit(1);
});
