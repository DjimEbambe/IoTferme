import createHttpError from 'http-errors';

export class ServiceError extends Error {
  constructor(message, { status = 500, code = 'UNEXPECTED', details } = {}) {
    super(message);
    this.name = 'ServiceError';
    this.status = status;
    this.statusCode = status;
    this.code = code;
    this.details = details;
  }
}

export const asyncHandler = (fn) => async (req, res, next) => {
  try {
    await fn(req, res, next);
  } catch (error) {
    next(error);
  }
};

export const notFound = () => createHttpError(404, 'Resource not found');

export const formatErrorResponse = (error, req) => {
  const status = error.status || error.statusCode || 500;
  const payload = {
    error: error.message || 'Unexpected error',
    code: error.code || (status >= 500 ? 'SERVER_ERROR' : 'REQUEST_ERROR'),
    requestId: req.id,
  };
  if (process.env.NODE_ENV !== 'production' && error.details) {
    payload.details = error.details;
  }
  return { status, payload };
};
