import http from 'http';
import { createApp } from './app.js';
import { env } from './config/env.js';
import { logger } from './logger.js';
import { initSocket } from './sockets/io.js';

const bootstrap = async () => {
  try {
    const { app, sessionMiddleware } = await createApp();
    const server = http.createServer(app);
    initSocket(server, sessionMiddleware);
    server.listen(env.port, () => {
      logger.info({ port: env.port }, 'Web gateway listening');
    });
  } catch (error) {
    logger.error({ err: error }, 'Failed to bootstrap web gateway');
    process.exit(1);
  }
};

bootstrap();
