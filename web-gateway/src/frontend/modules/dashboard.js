import { initTimelineChart } from './charts.js';

const sanitize = (str = '') => String(str ?? '').toLowerCase();

export const initDashboard = ({ showToast, dayjs }) => {
  if (!window.__DASHBOARD__) return null;

  const metricCards = document.querySelectorAll('.metric-card[data-metric]');
  const telemetryFeed = document.getElementById('telemetryFeed');
  const incidentList = document.getElementById('incidentList');
  const resourceContainer = document.getElementById('resourceList');
  const timeline = initTimelineChart(window.__DASHBOARD__.timeline || []);
  const lotsTable = document.getElementById('lotsTable');
  const filterButtons = Array.from(document.querySelectorAll('[data-filter]'));
  const sortButtons = Array.from(document.querySelectorAll('[data-sort]'));
  const lotRows = lotsTable ? Array.from(lotsTable.querySelectorAll('tbody tr')) : [];

  const updateKpis = (kpis) => {
    if (!kpis) return;
    metricCards.forEach((card) => {
      const key = card.dataset.metric;
      const valueNode = card.querySelector('.metric-value');
      if (!key || !valueNode) return;
      const value = kpis[key];
      if (value === undefined || value === null) return;
      if (typeof value === 'number') {
        valueNode.textContent = key === 'fcr' || key.includes('kwh') ? Number(value).toFixed(2) : Math.round(value).toString();
      } else {
        valueNode.textContent = value;
      }
    });
    const refreshNode = document.getElementById('lastRefresh');
    if (refreshNode) {
      const stamp = new Date().toLocaleString();
      refreshNode.textContent = stamp;
    }
  };

  const appendTelemetry = (msg) => {
    if (!telemetryFeed) return;
    const item = document.createElement('li');
    item.className = 'timeline-item flex-column';
    const timestamp = msg.ts ? new Date(msg.ts).toLocaleTimeString() : new Date().toLocaleTimeString();
    item.innerHTML = `
      <div class="d-flex justify-content-between w-100">
        <strong>${msg.asset_id || msg.device || 'Asset'}</strong>
        <span class="small text-secondary">${timestamp}</span>
      </div>
      <div class="small text-secondary">${JSON.stringify(msg.metrics || msg)}</div>`;
    telemetryFeed.prepend(item);
    while (telemetryFeed.children.length > 12) {
      telemetryFeed.removeChild(telemetryFeed.lastChild);
    }
  };

  const renderResources = (resources = []) => {
    if (!resourceContainer) return;
    resourceContainer.innerHTML = '';
    resources.forEach((resource) => {
      const reorder = resource.reorder_point || 1;
      const ratio = Math.min(100, Math.round((resource.stock_level / reorder) * 100));
      const wrapper = document.createElement('div');
      wrapper.className = 'mb-3 p-2 rounded-3';
      wrapper.style.background = 'rgba(255,255,255,0.03)';
      wrapper.innerHTML = `
        <div class="d-flex justify-content-between align-items-center">
          <div>
            <strong>${resource.name}</strong>
            <div class="small text-secondary text-uppercase">${resource.category}</div>
          </div>
          <span class="badge-soft ${resource.below_reorder ? 'danger' : 'success'}">${resource.stock_level} ${resource.unit}</span>
        </div>
        <div class="progress mt-2" style="height:6px;">
          <div class="progress-bar ${resource.below_reorder ? 'bg-danger' : 'bg-success'}" style="width:${ratio}%"></div>
        </div>
        ${resource.below_reorder ? '<small class="text-danger d-block mt-1"><i class="bi bi-exclamation-triangle me-1"></i>Recompléter sous 24h</small>' : ''}
      `;
      resourceContainer.appendChild(wrapper);
    });
  };

  const appendIncident = (incident) => {
    if (!incidentList) return;
    const item = document.createElement('li');
    item.className = 'timeline-item flex-column';
    const ts = incident.ts ? new Date(incident.ts).toLocaleTimeString() : new Date().toLocaleTimeString();
    item.innerHTML = `
      <div class="d-flex justify-content-between w-100">
        <strong>${incident.asset_id || incident.type}</strong>
        <span class="small text-secondary">${ts}</span>
      </div>
      <div class="small text-secondary">${incident.message || 'Incident signalé'} (${incident.type || ''})</div>`;
    incidentList.prepend(item);
    while (incidentList.children.length > 10) incidentList.removeChild(incidentList.lastChild);
  };

  if (filterButtons.length && lotRows.length) {
    filterButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        filterButtons.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        const filter = btn.dataset.filter;
        lotRows.forEach((row) => {
          row.hidden = filter !== 'all' && row.dataset.status !== filter;
        });
      });
    });
  }

  if (sortButtons.length && lotRows.length) {
    sortButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        sortButtons.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        const key = btn.dataset.sort;
        const sorted = lotRows.sort((a, b) => {
          const lotA = JSON.parse(a.dataset.lot || '{}');
          const lotB = JSON.parse(b.dataset.lot || '{}');
          if (key === 'age') return (lotB.age_days || 0) - (lotA.age_days || 0);
          if (key === 'fcr') return (lotA.kpis?.fcr || 0) - (lotB.kpis?.fcr || 0);
          return sanitize(lotA.lot_id).localeCompare(sanitize(lotB.lot_id));
        });
        const tbody = lotsTable.querySelector('tbody');
        sorted.forEach((row) => tbody.appendChild(row));
      });
    });
  }

  document.getElementById('btnReplay')?.addEventListener('click', () => {
    document.dispatchEvent(new Event('dashboard:replay'));
  });

  document.getElementById('btnClearFeed')?.addEventListener('click', () => {
    if (telemetryFeed) telemetryFeed.innerHTML = '<li class="text-secondary">Flux effacé</li>';
  });

  updateKpis(window.__DASHBOARD__.kpis || {});
  renderResources(window.__DASHBOARD__.resources || []);

  return {
    updateKpis,
    updateResources: renderResources,
    appendTelemetry,
    appendIncident,
    timeline,
  };
};
