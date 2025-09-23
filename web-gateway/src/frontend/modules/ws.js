import { io } from 'socket.io-client';

export const initSockets = ({ showToast, dashboard }) => {
  if (typeof io === 'undefined') return;
  const socket = io({ path: '/ws', withCredentials: true });
  const statusPill = document.getElementById('realtimeStatus');

  const updateStatus = (connected) => {
    if (!statusPill) return;
    statusPill.dataset.state = connected ? 'online' : 'offline';
    statusPill.querySelector('.label').textContent = connected ? 'Temps réel' : 'Déconnecté';
  };

  socket.on('connect', () => updateStatus(true));
  socket.on('disconnect', () => updateStatus(false));
  socket.on('connect_error', (err) => {
    console.error('[socket] error', err.message);
    updateStatus(false);
  });

  socket.on('telemetry.update', (msg) => {
    dashboard?.appendTelemetry?.(msg);
  });

  socket.on('kpi.update', (msg) => {
    dashboard?.updateKpis?.(msg.kpis || {});
    dashboard?.updateResources?.(msg.resources || []);
  });

  socket.on('incident.new', (incident) => {
    dashboard?.appendIncident?.(incident);
    showToast?.('Incident', incident.message || incident.type, 'warning');
    dashboard?.timeline?.append?.({
      ts: incident.ts || new Date().toISOString(),
      type: 'incident.alert',
      payload: incident,
    });
  });

  socket.on('cmd.ack', (ack) => {
    showToast?.('Commande', ack.ok ? 'Action confirmée' : ack.message || 'Commande rejetée', ack.ok ? 'success' : 'danger');
  });
};
