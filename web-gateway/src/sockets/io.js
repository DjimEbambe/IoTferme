import { Server } from 'socket.io';
import { computeUserRooms } from './rooms.js';
import { logger } from '../logger.js';
import { registerRedisBridge } from './events.js';

export const initSocket = (httpServer, sessionMiddleware) => {
  const io = new Server(httpServer, {
    cors: { origin: true, credentials: true },
    path: '/ws',
  });

  io.use((socket, next) => {
    sessionMiddleware(socket.request, {}, next);
  });

  io.use((socket, next) => {
    const user = socket.request.session?.passport?.user;
    if (!user) {
      return next(new Error('Unauthorized'));
    }
    socket.user = user;
    return next();
  });

  io.on('connection', (socket) => {
    const rooms = computeUserRooms(socket.user);
    rooms.forEach((room) => socket.join(room));
    socket.emit('auth.ok', { user: socket.user, rooms });

    socket.on('join.scope', (scope) => {
      const room = scope?.room;
      if (room) {
        socket.join(room);
      }
    });

    socket.on('disconnect', (reason) => {
      logger.info({ user: socket.user?.email, reason }, 'Socket disconnected');
    });
  });

  registerRedisBridge(io);

  return io;
};
