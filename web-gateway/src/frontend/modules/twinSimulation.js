const ASSET_LABELS = {
  'BLDG:A': 'Bâtiment 1',
  'BLDG:B': 'Bâtiment 2',
  'SUP:PROV': 'Couveuse',
};

const normalizeKey = (value) => {
  if (value === undefined || value === null) return '';
  return value
    .toString()
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .toUpperCase()
    .replace(/[^A-Z0-9]/g, '');
};

const CANONICAL_LOOKUP = {
  BLDGA: 'BLDG:A',
  BATIMENT1: 'BLDG:A',
  BAT1: 'BLDG:A',
  BATIMENTA: 'BLDG:A',
  BLDGB: 'BLDG:B',
  BATIMENT2: 'BLDG:B',
  BAT2: 'BLDG:B',
  BATIMENTB: 'BLDG:B',
  SUPPROV: 'SUP:PROV',
  COUVEUSE: 'SUP:PROV',
};

const canonicalAssetId = (asset) => {
  if (!asset) return null;
  const candidates = [asset.asset_id, asset?.metadata?.display_name, asset?.label, asset?.name];
  for (const candidate of candidates) {
    const key = normalizeKey(candidate);
    const canonical = CANONICAL_LOOKUP[key];
    if (canonical && ASSET_LABELS[canonical]) return canonical;
  }
  return null;
};

const loopRange = (value, min, max) => Math.max(min, Math.min(max, value));

const REGION = {
  timezoneOffsetMinutes: 60, // Africa/Kinshasa (UTC+1)
  outdoor: {
    temperature: { dawn: 22.5, midday: 32.5, night: 24.5 },
    humidity: { dawn: 88, midday: 62, night: 78 },
    air_quality: { base: 94, middayDip: 8 },
  },
};

const BUILDING_PROFILES = {
  'BLDG:A': {
    label: 'Bâtiment 1',
    insulationFactor: 0.55,
    humidityOffset: 0,
    airQualityOffset: 0,
    ranges: {
      temperature: { min: 23, max: 32 },
      humidity: { min: 50, max: 72 },
      air_quality: { min: 78, max: 95 },
    },
  },
  'BLDG:B': {
    label: 'Bâtiment 2',
    insulationFactor: 0.65,
    humidityOffset: -4,
    airQualityOffset: -2,
    ranges: {
      temperature: { min: 23, max: 31 },
      humidity: { min: 48, max: 70 },
      air_quality: { min: 76, max: 93 },
    },
  },
};

const COUVEUSE_PROFILE = {
  label: 'Couveuse',
  temperature: { min: 36.2, max: 38.2 },
  humidity: { min: 54, max: 62 },
  ranges: {
    temperature: { min: 35.5, max: 39 },
    humidity: { min: 50, max: 65 },
  },
};

const computeTimeFactors = () => {
  const now = new Date();
  const local = new Date(now.getTime() + REGION.timezoneOffsetMinutes * 60000 - now.getTimezoneOffset() * 60000);
  const hours = local.getUTCHours() + local.getUTCMinutes() / 60;
  const dayProgress = (hours / 24) * Math.PI * 2;
  return {
    hours,
    dayWave: Math.sin(dayProgress - Math.PI / 2), // -1 à 1, pic vers 14h locale
    inverseWave: Math.cos(dayProgress - Math.PI / 2),
  };
};

const interpolate = (wave, low, high) => low + ((wave + 1) / 2) * (high - low);

const outdoorConditions = () => {
  const { dayWave, inverseWave } = computeTimeFactors();
  const temp = interpolate(dayWave, REGION.outdoor.temperature.dawn, REGION.outdoor.temperature.midday);
  const humidity = interpolate(inverseWave, REGION.outdoor.humidity.midday, REGION.outdoor.humidity.dawn);
  const airQuality = REGION.outdoor.air_quality.base - Math.max(dayWave, 0) * REGION.outdoor.air_quality.middayDip;
  return { temp, humidity, airQuality };
};

const drift = (current, target, noise = 0.3) => {
  const variation = (Math.random() - 0.5) * noise;
  const towardTarget = (target - current) * 0.12;
  return current + towardTarget + variation;
};

const canonicalizeAsset = (asset) => {
  const canonical = canonicalAssetId(asset);
  if (!canonical) return null;
  return {
    ...asset,
    asset_id: canonical,
    metadata: {
      ...(asset.metadata || {}),
      display_name: ASSET_LABELS[canonical] || asset.metadata?.display_name,
    },
    metrics: { ...(asset.metrics || {}), ...(asset.telemetry || {}) },
    telemetry: { ...(asset.metrics || {}), ...(asset.telemetry || {}) },
  };
};

const updateBuilding = (asset, profile, environment) => {
  const metrics = asset.metrics || {};
  const interiorTarget = environment.temp * profile.insulationFactor + 24.5 * (1 - profile.insulationFactor);
  const temp = drift(metrics.temperature ?? interiorTarget, interiorTarget, 0.35);

  const humidityTarget = environment.humidity + profile.humidityOffset;
  const humidity = drift(metrics.humidity ?? humidityTarget, humidityTarget, 0.5);

  const airQualityBase = environment.airQuality + profile.airQualityOffset;
  const occupancyNoise = (Math.random() - 0.5) * 2;
  const airQuality = loopRange((metrics.air_quality ?? airQualityBase) + occupancyNoise, profile.ranges.air_quality.min, profile.ranges.air_quality.max);

  asset.metrics = {
    temperature: loopRange(temp, profile.ranges.temperature.min, profile.ranges.temperature.max),
    humidity: loopRange(humidity, profile.ranges.humidity.min, profile.ranges.humidity.max),
    air_quality: airQuality,
  };
  asset.telemetry = { ...asset.metrics };
  asset.metricRanges = profile.ranges;
};

const updateCouveuse = (asset) => {
  const metrics = asset.metrics || {};
  const temp = drift(metrics.temperature ?? 37.2, 37.2, 0.18);
  const humidity = drift(metrics.humidity ?? 58, 58, 0.25);
  asset.metrics = {
    temperature: loopRange(temp, COUVEUSE_PROFILE.ranges.temperature.min, COUVEUSE_PROFILE.ranges.temperature.max),
    humidity: loopRange(humidity, COUVEUSE_PROFILE.ranges.humidity.min, COUVEUSE_PROFILE.ranges.humidity.max),
  };
  asset.telemetry = { ...asset.metrics };
  asset.metricRanges = COUVEUSE_PROFILE.ranges;
};

export const SIMULATION_ENABLED = true;

export const startTwinSimulation = ({ twin, onUpdate, interval = 4000 }) => {
  if (!SIMULATION_ENABLED) return { stop: () => undefined };

  const assets = (twin?.assets || [])
    .map(canonicalizeAsset)
    .filter((asset) => asset && ['BLDG:A', 'BLDG:B', 'SUP:PROV'].includes(asset.asset_id));

  const tick = () => {
    const env = outdoorConditions();
    assets.forEach((asset) => {
      if (asset.asset_id === 'SUP:PROV') {
        updateCouveuse(asset);
        return;
      }
      const targetProfile = BUILDING_PROFILES[asset.asset_id];
      if (!targetProfile) return;
      updateBuilding(asset, targetProfile, env);
    });
    onUpdate?.({ assets: assets.map((asset) => ({ ...asset, metrics: { ...asset.metrics }, telemetry: { ...asset.telemetry } })), timestamp: Date.now() });
  };

  tick();
  const timer = setInterval(tick, interval);
  return {
    stop: () => clearInterval(timer),
  };
};

export default startTwinSimulation;
