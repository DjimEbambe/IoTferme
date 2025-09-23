import csurf from 'csurf';

export const csrfProtection = csurf({
  cookie: false,
  value: (req) => req.headers['csrf-token'] || req.body?._csrf || req.query?._csrf,
});

export const attachCsrfToken = (req, res, next) => {
  res.locals.csrfToken = req.csrfToken ? req.csrfToken() : undefined;
  next();
};
