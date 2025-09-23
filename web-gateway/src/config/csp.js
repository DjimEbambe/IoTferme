import crypto from 'crypto';
import helmet from 'helmet';

const scriptSrc = [
  "'self'",
  'https://cdn.jsdelivr.net',
  'https://cdnjs.cloudflare.com',
];

const styleSrc = [
  "'self'",
  'https://cdn.jsdelivr.net',
  'https://cdnjs.cloudflare.com',
  'https://fonts.googleapis.com',
  "'unsafe-inline'",
];

const fontSrc = [
  "'self'",
  'https://cdn.jsdelivr.net',
  'https://fonts.gstatic.com',
];

export const cspMiddleware = (nonceSecret) => {
  if (!nonceSecret) {
    throw new Error('CSP nonce secret is required');
  }

  return (req, res, next) => {
    const nonceSeed = `${nonceSecret}:${Date.now()}:${Math.random()}`;
    const nonce = crypto.createHash('sha256').update(nonceSeed).digest('base64');
    res.locals.cspNonce = nonce;
    helmet.contentSecurityPolicy({
      useDefaults: false,
      directives: {
        defaultSrc: ["'self'"],
        scriptSrc: [...scriptSrc, `'nonce-${nonce}'`],
        scriptSrcElem: [...scriptSrc, `'nonce-${nonce}'`],
        styleSrc,
        connectSrc: ["'self'", 'https://cdn.jsdelivr.net'],
        imgSrc: ["'self'", 'data:'],
        fontSrc,
        objectSrc: ["'none'"],
        frameSrc: ["'none'"],
        upgradeInsecureRequests: [],
      },
    })(req, res, next);
  };
};
