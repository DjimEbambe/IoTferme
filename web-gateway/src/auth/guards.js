import createHttpError from 'http-errors';
import { rbac } from './rbac.js';

export const requireAuth = (req, res, next) => {
  if (req.isAuthenticated && req.isAuthenticated()) {
    return next();
  }
  const wantsJson =
    req.xhr ||
    req.originalUrl.startsWith('/api') ||
    (req.get('Accept') || '').toLowerCase().includes('json');

  if (wantsJson) {
    return next(createHttpError(401, 'Authentication required'));
  }

  if (req.session) {
    req.session.returnTo = req.originalUrl;
  }
  return res.redirect('/login');
};

export const requirePermission = (permission, scope) => (req, res, next) => {
  if (!req.user || !rbac.hasPermission(req.user, permission)) {
    return next(createHttpError(403, 'Forbidden'));
  }
  if (scope && !rbac.hasScope(req.user, scope)) {
    return next(createHttpError(403, 'Insufficient scope'));
  }
  return next();
};

export const exposeUser = (req, res, next) => {
  res.locals.user = req.user;
  next();
};
