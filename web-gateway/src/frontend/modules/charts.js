import Chart from 'chart.js/auto';
import annotationPlugin from 'chartjs-plugin-annotation';

Chart.register(annotationPlugin);

export const initTimelineChart = (events = []) => {
  const canvas = document.getElementById('timelineChart');
  if (!canvas) return null;
  const labels = events.map((ev) => (ev?.ts ? new Date(ev.ts).toLocaleTimeString() : '—'));
  const importanceMap = {
    'lot.created': 3,
    'lot.updated': 2,
    'lot.closed': 4,
    'health.vaccine': 5,
    'incident.alert': 6,
  };

  const datasetValues = events.map((ev, idx) => importanceMap[ev?.type] || ((idx % 4) + 2));
  const pointColors = events.map((ev) => {
    if (!ev?.type) return 'rgba(95, 212, 172, 0.9)';
    if (ev.type.includes('incident')) return 'rgba(248, 115, 115, 0.9)';
    if (ev.type.includes('health')) return 'rgba(247, 186, 86, 0.9)';
    return 'rgba(95, 212, 172, 0.9)';
  });

  const gradient = (() => {
    const ctx = canvas.getContext('2d');
    const grad = ctx.createLinearGradient(0, 0, 0, 240);
    grad.addColorStop(0, 'rgba(95, 212, 172, 0.45)');
    grad.addColorStop(0.6, 'rgba(95, 212, 172, 0.16)');
    grad.addColorStop(1, 'rgba(95, 212, 172, 0)');
    return grad;
  })();

  const chart = new Chart(canvas, {
    type: 'line',
    data: {
      labels,
      datasets: [
        {
          label: 'Chronologie',
          data: datasetValues,
          borderColor: 'rgba(95, 212, 172, 0.6)',
          borderWidth: 2,
          tension: 0.35,
          fill: true,
          backgroundColor: gradient,
          pointRadius: 6,
          pointHoverRadius: 10,
          pointBackgroundColor: pointColors,
          pointBorderColor: 'rgba(255,255,255,0.6)',
          pointBorderWidth: 1.5,
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: true,
          grid: { color: 'rgba(255,255,255,0.05)' },
          ticks: { display: false },
        },
        x: {
          grid: { color: 'rgba(255,255,255,0.05)', drawOnChartArea: false },
          ticks: { color: 'rgba(255,255,255,0.6)' },
        },
      },
      plugins: {
        legend: { display: false },
        tooltip: {
          backgroundColor: 'rgba(12,15,28,0.9)',
          callbacks: {
            title: (items) => {
              const ev = events[items[0].dataIndex];
              const date = ev?.ts ? new Date(ev.ts) : null;
              return date ? date.toLocaleString() : 'Événement';
            },
            label: (item) => {
              const ev = events[item.dataIndex];
              return `${ev?.type || 'Événement'} › ${JSON.stringify(ev?.payload || {})}`;
            },
          },
        },
      },
    },
  });

  const append = (event) => {
    events.push(event);
    labels.push(event.ts ? new Date(event.ts).toLocaleTimeString() : '—');
    datasetValues.push(importanceMap[event.type] || 3);
    pointColors.push(event.type?.includes('incident') ? 'rgba(248,115,115,0.9)' : 'rgba(95,212,172,0.9)');
    chart.update('none');
  };

  const highlight = (index) => {
    const meta = chart.getDatasetMeta(0);
    const element = meta.data[index];
    if (!element) return;
    chart.setActiveElements([{ datasetIndex: 0, index }]);
    chart.tooltip.setActiveElements([{ datasetIndex: 0, index }], { x: element.x, y: element.y });
    chart.update();
  };

  document.addEventListener('dashboard:replay', () => {
    if (!events.length) return;
    let idx = 0;
    highlight(idx);
    const timer = setInterval(() => {
      idx += 1;
      if (idx >= events.length) {
        clearInterval(timer);
        chart.setActiveElements([]);
        chart.update();
        return;
      }
      highlight(idx);
    }, 1200);
  });

  return { append };
};
