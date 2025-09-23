export const initTwin = ({ showToast }) => {
  if (!window.__TWIN__) return null;
  import('./twinScene.js')
    .then(({ initTwinScene }) => {
      const sceneApi = initTwinScene({ twin: window.__TWIN__.twin, showToast });
      if (!sceneApi) return;
      window.__TwinScene = sceneApi;

      const heatmapBtn = document.getElementById('btnToggleHeatmap');
      const layoutBtn = document.getElementById('btnToggleLayout');
      const saveBtn = document.getElementById('btnLayoutSave');
      const calibrateBtn = document.getElementById('btnCalibrate');
      const calibrationForm = document.getElementById('calibrationForm');
      const twinForm = document.getElementById('formCmd');

      heatmapBtn?.addEventListener('click', () => {
        const mode = heatmapBtn.dataset.mode === 'temperature' ? 'humidity' : 'temperature';
        heatmapBtn.dataset.mode = mode;
        heatmapBtn.innerHTML = mode === 'temperature'
          ? '<i class="bi bi-thermometer-half me-2"></i>Heatmap Température'
          : '<i class="bi bi-cloud-drizzle me-2"></i>Heatmap Hygrométrie';
        sceneApi.toggleHeatmap(mode);
      });

      layoutBtn?.addEventListener('click', () => {
        const active = layoutBtn.classList.toggle('active');
        sceneApi.setLayoutMode(active);
        layoutBtn.innerHTML = active
          ? '<i class="bi bi-pencil-fill me-2"></i>Mode édition actif'
          : '<i class="bi bi-palette me-2"></i>Mode layout';
      });

      saveBtn?.addEventListener('click', () => sceneApi.saveLayout());

      calibrateBtn?.addEventListener('click', () => {
        const data = Object.fromEntries(new FormData(calibrationForm).entries());
        sceneApi.calibrate({
          ppId: data.pp_id,
          offsets: {
            'calibration.offset_temp': data.offset_temp ? Number(data.offset_temp) : undefined,
            'calibration.offset_hygro': data.offset_hygro ? Number(data.offset_hygro) : undefined,
          },
        });
        showToast?.('Calibration', `Offsets appliqués sur ${data.pp_id}`);
      });

      document.querySelectorAll('[data-focus-asset]').forEach((btn) => {
        btn.addEventListener('click', () => {
          sceneApi.focusAsset(btn.dataset.focusAsset);
        });
      });

      twinForm?.addEventListener('submit', async (evt) => {
        evt.preventDefault();
        const data = Object.fromEntries(new FormData(twinForm).entries());
        const payload = {
          site: 'SITE:KIN-GOLIATH',
          device: data.asset_id,
          asset_id: data.asset_id,
          relays: {},
        };
        switch (data.action) {
          case 'lamp_on':
            payload.relays.lamp = 'ON';
            break;
          case 'lamp_off':
            payload.relays.lamp = 'OFF';
            break;
          case 'fan_on':
            payload.relays.fan = 'ON';
            break;
          case 'door_open':
            payload.relays.door = 'ON';
            break;
          default:
            break;
        }
        try {
          const response = await fetch('/api/cmd', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'csrf-token': window.__TWIN__.csrfToken,
            },
            body: JSON.stringify(payload),
          });
          if (!response.ok) {
            const err = await response.json().catch(() => ({}));
            throw new Error(err.error || 'Erreur requête');
          }
          showToast?.('Commande', `Action ${data.action} envoyée à ${data.asset_id}`);
        } catch (error) {
          showToast?.('Commande', error.message, 'danger');
        }
      });
    })
    .catch((error) => {
      console.error('[twin] failed to initialize', error);
    });
};
