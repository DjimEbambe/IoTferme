import Offcanvas from 'bootstrap/js/dist/offcanvas.js';

export const initERP = ({ showToast, dayjs }) => {
  if (!window.__ERP__) return null;
  const searchInput = document.getElementById('lotSearch');
  const statusFilter = document.getElementById('lotStatusFilter');
  const table = document.getElementById('erpLotsTable');
  const exportBtn = document.getElementById('btnExportLots');
  const offcanvasEl = document.getElementById('lotDetailPane');
  const detailContent = document.getElementById('lotDetailContent');
  const rows = table ? Array.from(table.querySelectorAll('tbody tr')) : [];

  const showLotDetail = (lot) => {
    if (!detailContent) return;
    const kpis = lot.kpis || {};
    detailContent.innerHTML = `
      <div class="d-flex justify-content-between align-items-start mb-3">
        <div>
          <h4 class="fw-semibold">${lot.lot_id}</h4>
          <div class="text-secondary">Bâtiment ${lot.building_id || '-'} • Zone ${lot.zone_id || '-'}</div>
        </div>
        <span class="badge-soft ${lot.status === 'EN_COURS' ? 'erp-badge-active' : 'erp-badge-closed'}">${lot.status}</span>
      </div>
      <div class="glass-panel">
        <dl class="row mb-0">
          <dt class="col-6 text-secondary">Effectif actuel</dt><dd class="col-6">${lot.nb_current}</dd>
          <dt class="col-6 text-secondary">Effectif initial</dt><dd class="col-6">${lot.nb_initial}</dd>
          <dt class="col-6 text-secondary">Âge (jours)</dt><dd class="col-6">${lot.age_days ?? 'n/a'}</dd>
          <dt class="col-6 text-secondary">GMQ</dt><dd class="col-6">${kpis.gmq ?? 'n/a'}</dd>
          <dt class="col-6 text-secondary">FCR</dt><dd class="col-6">${kpis.fcr ?? 'n/a'}</dd>
        </dl>
      </div>
      <div class="mt-3">
        <h6 class="text-secondary text-uppercase">Notes</h6>
        <p class="small">${lot.notes || 'Aucune note saisie.'}</p>
      </div>`;
    Offcanvas.getOrCreateInstance(offcanvasEl).show();
  };

  rows.forEach((row) => {
    const data = JSON.parse(row.dataset.lot || '{}');
    row.addEventListener('click', () => showLotDetail(data));
  });

  const filterLots = () => {
    const query = (searchInput?.value || '').toLowerCase();
    const status = statusFilter?.value || 'all';
    rows.forEach((row) => {
      const data = JSON.parse(row.dataset.lot || '{}');
      const matchesStatus = status === 'all' || data.status === status;
      const matchesQuery = Object.values(data)
        .flatMap((value) => (typeof value === 'object' ? Object.values(value) : value))
        .join(' ')
        .toLowerCase()
        .includes(query);
      row.hidden = !(matchesStatus && matchesQuery);
    });
  };

  searchInput?.addEventListener('input', filterLots);
  statusFilter?.addEventListener('change', filterLots);

  exportBtn?.addEventListener('click', () => {
    const lots = window.__ERP__.lots || [];
    const header = ['lot_id', 'building_id', 'zone_id', 'nb_initial', 'nb_current', 'status', 'gmq', 'fcr'];
    const csvRows = [header.join(',')];
    lots.forEach((lot) => {
      const kpis = lot.kpis || {};
      csvRows.push([
        lot.lot_id,
        lot.building_id ?? '',
        lot.zone_id ?? '',
        lot.nb_initial ?? '',
        lot.nb_current ?? '',
        lot.status ?? '',
        kpis.gmq ?? '',
        kpis.fcr ?? '',
      ].join(','));
    });
    const blob = new Blob([csvRows.join('\n')], { type: 'text/csv;charset=utf-8;' });
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = `lots-${dayjs ? dayjs().format('YYYYMMDD-HHmmss') : Date.now()}.csv`;
    link.click();
  });

  document.getElementById('btnScheduleHealth')?.addEventListener('click', () => {
    showToast?.('Santé', 'Visite sanitaire planifiée - vétérinaire notifié.');
  });

  document.getElementById('btnScheduleFeed')?.addEventListener('click', () => {
    showToast?.('Approvisionnement', 'Commande provende générée pour SUP:PROV.');
  });

  document.getElementById('btnOpenLotOffcanvas')?.addEventListener('click', () => {
    const lots = window.__ERP__.lots || [];
    if (lots.length) showLotDetail(lots[0]);
  });

  const lotForm = document.getElementById('formLot');
  lotForm?.addEventListener('submit', async (evt) => {
    evt.preventDefault();
    const data = Object.fromEntries(new FormData(lotForm).entries());
    data.nb_initial = Number(data.nb_initial);
    data.nb_current = Number(data.nb_current);
    data.pp_ids = data.pp_ids ? data.pp_ids.split(',').map((s) => s.trim()).filter(Boolean) : [];
    try {
      const response = await fetch('/api/erp/lots', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'csrf-token': window.__ERP__.csrfToken,
        },
        body: JSON.stringify(data),
      });
      if (!response.ok) {
        const error = await response.json().catch(() => ({}));
        throw new Error(error.error || 'Erreur requête');
      }
      showToast?.('Lot', 'Nouveau lot enregistré');
      window.location.reload();
    } catch (error) {
      showToast?.('Lot', error.message, 'danger');
    }
  });

  return {
    filter: filterLots,
  };
};
