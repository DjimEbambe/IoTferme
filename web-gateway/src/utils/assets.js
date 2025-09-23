import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const manifestPath = path.join(__dirname, '..', 'public', 'dist', 'manifest.json');

let cachedManifest = null;
let cachedMtime = 0;

const loadManifest = () => {
  try {
    const stats = fs.statSync(manifestPath);
    if (!cachedManifest || stats.mtimeMs !== cachedMtime) {
      const raw = fs.readFileSync(manifestPath, 'utf-8');
      cachedManifest = JSON.parse(raw);
      cachedMtime = stats.mtimeMs;
    }
  } catch (error) {
    cachedManifest = null;
  }
  return cachedManifest || {};
};

export const asset = (name) => {
  const manifest = loadManifest();
  if (manifest[name]) return manifest[name];
  if (manifest[`js/${name}`]) return manifest[`js/${name}`];
  if (manifest[`css/${name}`]) return manifest[`css/${name}`];
  return `/dist/${name}`;
};

export const assetsMiddleware = (req, res, next) => {
  res.locals.asset = asset;
  next();
};
