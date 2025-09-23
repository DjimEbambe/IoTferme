import 'bootstrap/dist/js/bootstrap.bundle.js';
import Toast from 'bootstrap/js/dist/toast.js';
import Modal from 'bootstrap/js/dist/modal.js';
import dayjs from 'dayjs';
import utc from 'dayjs/plugin/utc.js';
import { initSockets } from './modules/ws.js';
import { initDashboard } from './modules/dashboard.js';
import { initERP } from './modules/erp.js';
import { initTwin } from './modules/twin.js';
import './styles/main.scss';

dayjs.extend(utc);

const state = {
  csrf: window.__DASHBOARD__?.csrfToken || window.__ERP__?.csrfToken || window.__TWIN__?.csrfToken,
  theme: localStorage.getItem('fs-theme') || 'dark',
  dashboard: null,
};

const qs = (selector) => document.querySelector(selector);
const qsa = (selector) => Array.from(document.querySelectorAll(selector));

const showToast = (title, message, variant = 'success') => {
  const stack = qs('#toastStack');
  if (!stack) return;
  const wrapper = document.createElement('div');
  wrapper.innerHTML = `
    <div class="toast app-toast align-items-center border-0 text-bg-${variant}" role="alert" aria-live="assertive" aria-atomic="true">
      <div class="d-flex">
        <div class="toast-body">
          <i class="bi bi-info-circle me-2"></i>
          <div>
            <strong class="d-block">${title}</strong>
            <span>${message}</span>
          </div>
        </div>
        <button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
      </div>
    </div>`;
  const toastEl = wrapper.firstElementChild;
  stack.appendChild(toastEl);
  const toast = new Toast(toastEl, { delay: 4000 });
  toast.show();
};

window.showToast = showToast;

const highlightNav = () => {
  const path = window.location.pathname;
  qsa('.app-nav-tabs .nav-link').forEach((link) => {
    const isDashboard = path === '/' && link.dataset.nav === 'dashboard';
    const isERP = path.startsWith('/erp') && link.dataset.nav === 'erp';
    const isTwin = path.startsWith('/twins') && link.dataset.nav === 'twins';
    if (isDashboard || isERP || isTwin) link.classList.add('active');
    else link.classList.remove('active');
  });
};

const applyTheme = (mode) => {
  state.theme = mode;
  document.documentElement.setAttribute('data-bs-theme', mode);
  localStorage.setItem('fs-theme', mode);
  const icon = qs('#themeToggle i');
  if (icon) icon.className = mode === 'dark' ? 'bi bi-moon-stars' : 'bi bi-sun';
};

const initThemeToggle = () => {
  const toggle = qs('#themeToggle');
  if (!toggle) return;
  applyTheme(state.theme);
  toggle.addEventListener('click', () => applyTheme(state.theme === 'dark' ? 'light' : 'dark'));
};

const initClock = () => {
  const clock = qs('#tzClock');
  if (!clock) return;
  const tick = () => {
    const now = dayjs().utc().add(1, 'hour');
    clock.textContent = now.format('DD MMM YYYY • HH:mm:ss');
  };
  tick();
  setInterval(tick, 1000);
};

const initQuickCommandModal = () => {
  const modalEl = document.getElementById('quickCommandModal');
  if (!modalEl) return;
  const form = modalEl.querySelector('#quickCmdForm');
  const submit = modalEl.querySelector('#btnSendQuickCmd');
  submit?.addEventListener('click', async () => {
    const data = Object.fromEntries(new FormData(form).entries());
    if (!data.asset_id) {
      showToast('Commande', 'Veuillez sélectionner un asset valide', 'warning');
      return;
    }
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
      const res = await fetch('/api/cmd', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'csrf-token': state.csrf,
        },
        body: JSON.stringify(payload),
      });
      if (!res.ok) {
        const err = await res.json().catch(() => ({}));
        throw new Error(err.error || 'Erreur requête');
      }
      showToast('Commande envoyée', `Action ${data.action} envoyée à ${data.asset_id}`);
      Modal.getOrCreateInstance(modalEl).hide();
    } catch (error) {
      showToast('Commande', error.message, 'danger');
    }
  });
};

highlightNav();
initThemeToggle();
initClock();
initQuickCommandModal();

state.dashboard = initDashboard({ showToast, dayjs });
const erpApi = initERP({ showToast, dayjs });
initTwin({ showToast, dayjs });
initSockets({ showToast, dashboard: state.dashboard });

const params = new URLSearchParams(window.location.search);
if (params.get('welcome')) {
  showToast('Bienvenue', 'Votre compte est prêt, bon suivi !');
  params.delete('welcome');
  const query = params.toString();
  const newUrl = `${window.location.pathname}${query ? `?${query}` : ''}${window.location.hash}`;
  window.history.replaceState({}, '', newUrl);
}

window.__ERP_API__ = erpApi;
