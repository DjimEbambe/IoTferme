import path from 'path';
import url from 'url';
import { randomUUID } from 'crypto';
import express from 'express';
import expressLayouts from 'express-ejs-layouts';
import session from 'express-session';
import RedisStore from 'connect-redis';
import compression from 'compression';
import rateLimit from 'express-rate-limit';
import helmet from 'helmet';
import pinoHttp from 'pino-http';
import bcrypt from 'bcrypt';

import { env, isProd } from './config/env.js';
import { cspMiddleware } from './config/csp.js';
import { csrfProtection, attachCsrfToken } from './config/csrf.js';
import { passport } from './auth/passport.js';
import { requireAuth, exposeUser } from './auth/guards.js';
import { logger } from './logger.js';
import { getRedisClient } from './redis.js';
import { createConnection, UserModel } from './dao/models/common.js';
import dashboardRouter from './routes/pages/dashboard.js';
import erpRouter from './routes/pages/erp.js';
import twinsRouter from './routes/pages/twins.js';
import kpiApi from './routes/api/kpi.js';
import seriesApi from './routes/api/series.js';
import cmdApi from './routes/api/cmd.js';
import paramsApi from './routes/api/params.js';
import erpApi from './routes/api/erp.js';
import mlProxyApi from './routes/api/ml-proxy.js';
import { seedInitialData } from './dao/seed/init.seed.js';
import { assetsMiddleware } from './utils/assets.js';
import { asyncHandler, formatErrorResponse, notFound } from './utils/errors.js';
import { signupSchema, sanitizeSignupPayload } from './auth/validators.js';

const __dirname = path.dirname(url.fileURLToPath(import.meta.url));

process.env.TZ = env.timezone;

export const createApp = async () => {
  await createConnection(env.mongoUri, env.mongoUser, env.mongoPassword);
  if (!isProd) {
    await seedInitialData();
  }

  const app = express();
  app.set('trust proxy', 1);
  app.set('view engine', 'ejs');
  app.set('views', path.join(__dirname, 'views'));
  app.use(expressLayouts);
  app.set('layout', 'layout');

  const redisClient = getRedisClient();
  const redisStore = new RedisStore({ client: redisClient, prefix: 'sess:' });

  const sessionMiddleware = session({
    secret: env.sessionSecret,
    resave: false,
    saveUninitialized: false,
    store: redisStore,
    cookie: {
      httpOnly: true,
      secure: isProd,
      sameSite: 'lax',
      maxAge: 1000 * 60 * 60 * 24,
    },
  });

  app.use(
    pinoHttp({
      logger,
      genReqId: (req) => req.id ?? randomUUID(),
      customProps: (req) => ({ requestId: req.id }),
      customSuccessMessage: (req, res) => `${req.method} ${req.originalUrl} completed`,
      customErrorMessage: (req, res, err) => `${req.method} ${req.originalUrl} failed`,
    })
  );

  app.use((req, res, next) => {
    if (req.id) {
      res.setHeader('X-Request-ID', req.id);
    }
    res.locals.requestId = req.id;
    next();
  });
  app.use(helmet.hsts({ maxAge: 15552000, includeSubDomains: true, preload: false, setIf: () => isProd }));
  app.use(cspMiddleware(env.cspNonceSecret));
  app.use(helmet.hidePoweredBy());
  app.use(compression());
  app.use(express.json({ limit: '1mb' }));
  app.use(express.urlencoded({ extended: true }));
  app.use(
    rateLimit({
      windowMs: env.rateLimit.windowMs,
      max: env.rateLimit.max,
      standardHeaders: true,
      legacyHeaders: false,
    })
  );

  app.use(sessionMiddleware);
  app.use(passport.initialize());
  app.use(passport.session());
  app.use(exposeUser);

  app.use(express.static(path.join(__dirname, 'public'), { maxAge: isProd ? '7d' : 0 }));
  app.use(assetsMiddleware);

  app.use(csrfProtection);
  app.use(attachCsrfToken);

  app.get('/healthz', (req, res) => {
    res.json({ status: 'ok', at: new Date().toISOString() });
  });

  app.get('/login', (req, res) => {
    if (req.isAuthenticated?.()) {
      return res.redirect('/');
    }
    const { error, success } = req.query;
    const errorLookup = {
      invalid: 'Identifiants invalides. Vérifiez votre email ou numéro.',
      unauthorized: 'Veuillez vous connecter pour poursuivre.',
    };
    const successLookup = {
      logout: 'Vous êtes déconnecté.',
    };
    return res.render('auth/login', {
      csrfToken: res.locals.csrfToken,
      nonce: res.locals.cspNonce,
      title: 'Connexion',
      errorMessage: errorLookup[error] || null,
      successMessage: successLookup[success] || null,
    });
  });

  app.get('/signup', (req, res) => {
    if (req.isAuthenticated?.()) {
      return res.redirect('/');
    }
    res.render('auth/signup', {
      title: 'Inscription',
      csrfToken: res.locals.csrfToken,
      nonce: res.locals.cspNonce,
      formData: {},
      errors: [],
      successMessage: null,
    });
  });

  app.post('/signup', asyncHandler(async (req, res) => {
    const { value, error } = signupSchema.validate(req.body, {
      abortEarly: false,
      stripUnknown: true,
    });

    if (error) {
      const messages = error.details.map((detail) => detail.message);
      return res.status(400).render('auth/signup', {
        title: 'Inscription',
        csrfToken: res.locals.csrfToken,
        nonce: res.locals.cspNonce,
        errors: messages,
        formData: {
          displayName: req.body.displayName,
          email: req.body.email,
          phone: req.body.phone,
        },
      });
    }

    const { email, phone, displayName, password } = sanitizeSignupPayload(value);
    const passwordHash = await bcrypt.hash(password, 12);
    const userDoc = new UserModel({
      email: email || undefined,
      phone: phone || undefined,
      passwordHash,
      displayName:
        displayName || email?.split('@')[0] || `user-${Math.random().toString(16).slice(2, 6)}`,
      roles: [env.rbacDefaultRole],
    });

    try {
      await userDoc.save();
    } catch (err) {
      if (err?.code === 11000) {
        const duplicatedField = Object.keys(err.keyPattern || { email: 1 })[0];
        const msg = duplicatedField === 'phone' ? 'Ce numéro est déjà utilisé.' : 'Cet email est déjà utilisé.';
        return res.status(409).render('auth/signup', {
          title: 'Inscription',
          csrfToken: res.locals.csrfToken,
          nonce: res.locals.cspNonce,
          errors: [msg],
          formData: {
            displayName,
            email,
            phone: req.body.phone,
          },
        });
      }
      throw err;
    }

    await new Promise((resolve, reject) => {
      req.logIn(userDoc, (loginErr) => {
        if (loginErr) reject(loginErr);
        else resolve();
      });
    });

    const redirectTo = req.session?.returnTo || '/';
    if (req.session) {
      delete req.session.returnTo;
    }
    const separator = redirectTo.includes('?') ? (redirectTo.endsWith('?') || redirectTo.endsWith('&') ? '' : '&') : '?';
    return res.redirect(`${redirectTo}${separator}welcome=1`);
  }));

  app.post('/login', (req, res, next) => {
    passport.authenticate('local', (err, user, info) => {
      if (err) return next(err);
      if (!user) {
        return res.redirect('/login?error=invalid');
      }
      req.logIn(user, (loginErr) => {
        if (loginErr) return next(loginErr);
        const redirectTo = req.session?.returnTo || '/';
        if (req.session) {
          delete req.session.returnTo;
        }
        return res.redirect(redirectTo);
      });
    })(req, res, next);
  });

  app.post('/logout', requireAuth, (req, res, next) => {
    req.logout((err) => {
      if (err) return next(err);
      req.session.destroy(() => {
        res.redirect('/login?success=logout');
      });
    });
  });

  app.use('/', requireAuth, dashboardRouter);
  app.use('/erp', requireAuth, erpRouter);
  app.use('/twins', requireAuth, twinsRouter);

  app.use('/api/kpi', requireAuth, kpiApi);
  app.use('/api/series', requireAuth, seriesApi);
  app.use('/api/cmd', requireAuth, cmdApi);
  app.use('/api/params', requireAuth, paramsApi);
  app.use('/api/erp', requireAuth, erpApi);
  app.use('/api/ml', requireAuth, mlProxyApi);

  app.use((req, res, next) => {
    next(notFound());
  });

  app.use((err, req, res, next) => {
    if (res.headersSent) {
      return next(err);
    }

    const { status, payload } = formatErrorResponse(err, req);
    logger.error(
      { err, status, path: req.originalUrl, requestId: req.id },
      'Request error'
    );

    if (req.originalUrl.startsWith('/api') || req.accepts('json')) {
      return res.status(status).json(payload);
    }

    return res.status(status).render('errors/error', {
      status,
      message: payload.error,
      code: payload.code,
      requestId: payload.requestId,
      stack: isProd ? undefined : err.stack,
      nonce: res.locals.cspNonce,
      title: `Erreur ${status}`,
    });
  });

  return { app, sessionMiddleware };
};
