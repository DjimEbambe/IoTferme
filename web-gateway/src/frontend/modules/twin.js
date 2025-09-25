import { SIMULATION_ENABLED, startTwinSimulation } from './twinSimulation.js';

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

const HEATMAP_SEQUENCE = ['temperature', 'humidity', 'air_quality'];
const HEATMAP_TEXT = {
  temperature: '<i class="bi bi-thermometer-half me-2"></i>Température',
  humidity: '<i class="bi bi-cloud-drizzle me-2"></i>Humidité',
  air_quality: '<i class="bi bi-wind me-2"></i>Qualité d\'air',
};

const METRIC_LABELS = {
  temperature: 'Température',
  humidity: 'Humidité',
  air_quality: 'Qualité d\'air',
};

const METRIC_UNITS = {
  temperature: '°C',
  humidity: '%',
  air_quality: '%',
};

const ASSET_METRICS = {
  'BLDG:A': ['temperature', 'humidity', 'air_quality'],
  'BLDG:B': ['temperature', 'humidity', 'air_quality'],
  'SUP:PROV': ['temperature', 'humidity'],
};

const SUMMARY_METRICS = {
  'BLDG:A': ['temperature', 'humidity', 'air_quality'],
  'BLDG:B': ['temperature', 'humidity', 'air_quality'],
  'SUP:PROV': ['temperature', 'humidity'],
};

const formatMetric = (value, unit) => (typeof value === 'number' ? `${value.toFixed(1)}${unit}` : '–');

const canonicalAssetId = (asset) => {
  if (!asset) return null;
  const candidates = [asset.asset_id, asset?.metadata?.display_name, asset?.label, asset?.name];
  for (const candidate of candidates) {
    const key = normalizeKey(candidate);
    const canonical = CANONICAL_LOOKUP[key];
    if (canonical) return canonical;
  }
  return null;
};

const canonicalizeAsset = (asset) => {
  const canonical = canonicalAssetId(asset);
  if (!canonical || !ASSET_LABELS[canonical]) return null;
  return {
    ...asset,
    asset_id: canonical,
    metadata: { ...(asset.metadata || {}), display_name: ASSET_LABELS[canonical] },
  };
};

const getVisibleAssets = () => {
  const list = [];
  (window.__TWIN__?.twin?.assets || []).forEach((asset) => {
    const normalized = canonicalizeAsset(asset);
    if (!normalized || list.find((entry) => entry.asset_id === normalized.asset_id)) return;
    const combined = { ...(normalized.metrics || {}), ...(normalized.telemetry || {}) };
    list.push({ ...normalized, metrics: combined, telemetry: combined });
  });
  return list;
};

const renderMetricCards = (assetsOverride) => {
  const container = document.getElementById('liveMetrics');
  if (!container) return;

  const assets = assetsOverride || getVisibleAssets();
  if (!assets.length) {
    container.innerHTML = '<p class="text-secondary mb-0">Aucun équipement modélisé pour le moment.</p>';
    return;
  }

  const cards = assets
    .map((asset) => {
      const metrics = { ...(asset.metrics || {}), ...(asset.telemetry || {}) };
      const rows = (ASSET_METRICS[asset.asset_id] || [])
        .map((metric) => (
          `<div class="metric-row">
            <span>${METRIC_LABELS[metric]}</span>
            <strong>${formatMetric(metrics[metric], METRIC_UNITS[metric])}</strong>
          </div>`
        ))
        .join('');
      return `
        <div class="metric-card">
          <div class="metric-card__title">${ASSET_LABELS[asset.asset_id]}</div>
          <div class="metric-card__body">${rows}</div>
        </div>
      `;
    })
    .join('');

  container.innerHTML = cards;
};

const renderOverlaySummary = (assetsOverride) => {
  const container = document.getElementById('liveSummary');
  if (!container) return;

  const assets = assetsOverride || getVisibleAssets();
  if (!assets.length) {
    container.innerHTML = '<p class="text-secondary mb-0">Données en cours de synchronisation...</p>';
    return;
  }

  const rows = assets
    .map((asset) => {
      const metrics = { ...(asset.metrics || {}), ...(asset.telemetry || {}) };
      const summary = (SUMMARY_METRICS[asset.asset_id] || [])
        .map((metric) => `<span class="summary-pill">${formatMetric(metrics[metric], METRIC_UNITS[metric])}</span>`)
        .join('');
      return `
        <div class="summary-row">
          <span>${ASSET_LABELS[asset.asset_id]}</span>
          <span class="summary-values">${summary}</span>
        </div>
      `;
    })
    .join('');

  container.innerHTML = rows;
};

const populateCommandAssets = (assetsOverride) => {
  const select = document.getElementById('controlAsset');
  if (!select) return;
  const assets = assetsOverride || getVisibleAssets();
  select.innerHTML = assets
    .map((asset) => `<option value="${asset.asset_id}">${ASSET_LABELS[asset.asset_id]}</option>`)
    .join('');
};

export const initTwin = ({ showToast }) => {
  if (!window.__TWIN__) return null;
  import('./twinScene.js')
    .then(({ initTwinScene }) => {
      const sceneApi = initTwinScene({ twin: window.__TWIN__.twin, showToast });
      if (!sceneApi) return;
      window.__TwinScene = sceneApi;

      const initialAssets = getVisibleAssets();
      window.__TWIN__.twin.assets = initialAssets;
      sceneApi.syncAssets?.(initialAssets);
      sceneApi.refreshHeatmap?.();
      renderMetricCards(initialAssets);
      renderOverlaySummary(initialAssets);
      populateCommandAssets(initialAssets);

      const heatmapBtn = document.getElementById('btnToggleHeatmap');
      if (heatmapBtn) {
        const initialMode = HEATMAP_SEQUENCE.includes(heatmapBtn.dataset.mode)
          ? heatmapBtn.dataset.mode
          : HEATMAP_SEQUENCE[0];
        heatmapBtn.dataset.mode = initialMode;
        heatmapBtn.innerHTML = HEATMAP_TEXT[initialMode];
        heatmapBtn.addEventListener('click', () => {
          const current = heatmapBtn.dataset.mode || HEATMAP_SEQUENCE[0];
          const next = HEATMAP_SEQUENCE[(HEATMAP_SEQUENCE.indexOf(current) + 1) % HEATMAP_SEQUENCE.length];
          heatmapBtn.dataset.mode = next;
          heatmapBtn.innerHTML = HEATMAP_TEXT[next];
          sceneApi.toggleHeatmap(next);
        });
      }

      document.querySelectorAll('[data-focus-asset]').forEach((btn) => {
        btn.addEventListener('click', () => {
          sceneApi.focusAsset(btn.dataset.focusAsset);
        });
      });

      const layoutBtn = document.getElementById('btnToggleLayout');
      const saveBtn = document.getElementById('btnLayoutSave');
      layoutBtn?.addEventListener('click', () => {
        const active = layoutBtn.classList.toggle('active');
        sceneApi.setLayoutMode(active);
        layoutBtn.innerHTML = active
          ? '<i class="bi bi-pencil-fill me-2"></i>Repères actifs'
          : '<i class="bi bi-palette me-2"></i>Repères masqués';
      });
      saveBtn?.addEventListener('click', () => showToast?.('Jumeau', 'Sauvegarde en cours de développement')); // placeholder

      const commandForm = document.getElementById('comfortControls');
      commandForm?.addEventListener('submit', async (evt) => {
        evt.preventDefault();
        const form = new FormData(commandForm);
        const assetId = form.get('asset_id');
        const action = form.get('action');
        if (!assetId || !action) return;

        const canonical = CANONICAL_LOOKUP[normalizeKey(assetId)] || assetId;
        if (!ASSET_LABELS[canonical]) {
          showToast?.('Commande', 'Équipement inconnu', 'warning');
          return;
        }

        const payload = {
          site: 'SITE:KIN-GOLIATH',
          device: canonical,
          asset_id: canonical,
          relays: {},
        };

        if (action === 'lights_on') payload.relays.lamp = 'ON';
        if (action === 'lights_off') payload.relays.lamp = 'OFF';
        if (action === 'ventilation_on') payload.relays.fan = 'ON';
        if (action === 'ventilation_off') payload.relays.fan = 'OFF';
        if (action === 'doors_open') payload.relays.door = 'ON';
        if (action === 'doors_close') payload.relays.door = 'OFF';

        try {
          const response = await fetch('/api/cmd', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'csrf-token': window.__TWIN__.csrfToken,
            },
            body: JSON.stringify(payload),
          });
          if (!response.ok) throw new Error('Impossible d\'envoyer la commande');
          showToast?.('Commande', `${ASSET_LABELS[canonical]} · action envoyée`);
        } catch (error) {
          showToast?.('Commande', error.message || 'Erreur lors de l\'envoi', 'danger');
        }
      });

      let simulationHandle = null;
      if (SIMULATION_ENABLED) {
        simulationHandle = startTwinSimulation({
          twin: window.__TWIN__.twin,
          onUpdate: ({ assets, timestamp }) => {
            window.__TWIN__.twin.assets = assets;
            sceneApi.syncAssets?.(assets);
            sceneApi.refreshHeatmap?.();
            renderMetricCards(assets);
            renderOverlaySummary(assets);
            populateCommandAssets(assets);
            window.__TwinSceneState = { ...(window.__TwinSceneState || {}), lastSimulatedAt: timestamp };
          },
        });
        window.__TwinSimulation = simulationHandle;
      }

      window.addEventListener('beforeunload', () => simulationHandle?.stop?.());
    })
    .catch((error) => {
      console.error('[twin] failed to initialize', error);
    });
};
