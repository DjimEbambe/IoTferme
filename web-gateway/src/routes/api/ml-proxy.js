import { Router } from 'express';
import { env } from '../../config/env.js';
import { asyncHandler, ServiceError } from '../../utils/errors.js';

const router = Router();

const forward = async (path, payload) => {
  let response;
  try {
    response = await fetch(`${env.ml.baseUrl}${path}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-API-Key': env.ml.apiKey,
      },
      body: JSON.stringify(payload),
    });
  } catch (error) {
    throw new ServiceError('Impossible de joindre le moteur ML', {
      status: 502,
      code: 'ML_UPSTREAM_UNREACHABLE',
      details: error.message,
    });
  }
  const text = await response.text();
  if (!response.ok) {
    let details;
    try {
      details = JSON.parse(text);
    } catch (_) {
      details = text;
    }
    throw new ServiceError('Erreur moteur ML', {
      status: response.status,
      code: 'ML_UPSTREAM_ERROR',
      details,
    });
  }
  try {
    return text ? JSON.parse(text) : {};
  } catch (error) {
    throw new ServiceError('Réponse ML invalide', {
      status: 502,
      code: 'ML_INVALID_RESPONSE',
      details: text,
    });
  }
};

router.post('/anomaly', asyncHandler(async (req, res) => {
  const data = await forward('/ml/anomaly', req.body);
  res.json(data);
}));

router.post('/predict', asyncHandler(async (req, res) => {
  const data = await forward('/ml/predict', req.body);
  res.json(data);
}));

router.post('/optimize_ration', asyncHandler(async (req, res) => {
  const data = await forward('/ml/optimize_ration', req.body);
  res.json(data);
}));

router.post('/schedule', asyncHandler(async (req, res) => {
  const data = await forward('/ml/schedule', req.body);
  res.json(data);
}));

export default router;
