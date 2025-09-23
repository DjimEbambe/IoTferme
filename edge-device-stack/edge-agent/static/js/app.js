(async () => {
  const newCorrelationId = () => {
    const rand = typeof crypto !== 'undefined' && crypto.randomUUID
      ? crypto.randomUUID().slice(0, 8)
      : Math.random().toString(16).slice(2, 10);
    return `ui-manual-${Date.now()}-${rand}`;
  };

  const postJson = async (url, body) => {
    const response = await fetch(url, {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: body ? JSON.stringify(body) : undefined,
    });
    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || response.statusText);
    }
    return response.json();
  };

  const attachCommandForm = () => {
    const form = document.querySelector('#cmd-form');
    if (!form) return;
    const payloadField = document.querySelector('#cmd-payload');
    const resultBox = document.querySelector('#cmd-result');
    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      try {
        const payload = JSON.parse(payloadField.value);
        payload.correlation_id = newCorrelationId();
        const data = await postJson('/api/cmd', payload);
        resultBox.classList.remove('d-none', 'alert-danger');
        resultBox.classList.add('alert-success');
        resultBox.textContent = `ACK: ${JSON.stringify(data)}`;
      } catch (error) {
        resultBox.classList.remove('d-none', 'alert-success');
        resultBox.classList.add('alert-danger');
        resultBox.textContent = `Erreur: ${error.message}`;
      }
    });
  };

  const attachRelayForm = () => {
    const form = document.querySelector('#relay-form');
    if (!form) return;
    const resultBox = document.querySelector('#relay-result');
    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      const payload = {
        asset_id: document.querySelector('#relay-asset').value,
        channel: document.querySelector('#relay-channel').value || 'lamp',
        state: document.querySelector('#relay-state').value,
      };
      const duration = document.querySelector('#relay-duration').value;
      if (duration) payload.duration_s = Number(duration);
      try {
        const data = await postJson('/api/cmd/test-relay', payload);
        resultBox.classList.remove('d-none', 'alert-danger');
        resultBox.classList.add('alert-success');
        resultBox.textContent = `ACK: ${JSON.stringify(data)}`;
      } catch (error) {
        resultBox.classList.remove('d-none', 'alert-success');
        resultBox.classList.add('alert-danger');
        resultBox.textContent = `Erreur: ${error.message}`;
      }
    });
  };

  const attachBufferActions = () => {
    document.querySelectorAll('[data-action="replay"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        btn.disabled = true;
        try {
          await postJson('/api/buffer/replay');
          location.reload();
        } catch (error) {
          alert(error.message);
        } finally {
          btn.disabled = false;
        }
      });
    });
    document.querySelectorAll('[data-action="purge"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        if (!confirm('Confirmer la purge des messages ACK ?')) return;
        btn.disabled = true;
        try {
          await postJson('/api/buffer/purge');
          location.reload();
        } catch (error) {
          alert(error.message);
        } finally {
          btn.disabled = false;
        }
      });
    });
  };

  const attachDiagActions = () => {
    const resultBox = document.querySelector('#diag-result');
    const showResult = (message, success = true) => {
      if (!resultBox) return;
      resultBox.classList.remove('d-none', 'alert-success', 'alert-danger');
      resultBox.classList.add(success ? 'alert-success' : 'alert-danger');
      resultBox.textContent = message;
    };
    const macForm = document.querySelector('#set-mac-form');
    if (macForm) {
      macForm.addEventListener('submit', async (event) => {
        event.preventDefault();
        const macInput = document.querySelector('#set-mac-value');
        const persistInput = document.querySelector('#set-mac-persist');
        const mac = macInput.value.trim();
        if (!/^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$/.test(mac)) {
          showResult('MAC invalide', false);
          return;
        }
        macForm.querySelectorAll('button, input').forEach((el) => { el.disabled = true; });
        try {
          const res = await postJson('/api/diag/set-mac', {mac: mac.toLowerCase(), persist: persistInput.checked});
          showResult(`Commande MAC envoyée (${res.mac}${res.persist ? ', persistée' : ''})`);
        } catch (error) {
          showResult(error.message, false);
        } finally {
          macForm.querySelectorAll('button, input').forEach((el) => { el.disabled = false; });
        }
      });
    }
    document.querySelectorAll('[data-action="time-sync"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        btn.disabled = true;
        try {
          const res = await postJson('/api/diag/time-sync');
          showResult(`Time sync envoyé (${res.ts})`);
        } catch (error) {
          showResult(error.message, false);
        } finally {
          btn.disabled = false;
        }
      });
    });
    document.querySelectorAll('[data-action="pair-begin"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        btn.disabled = true;
        try {
          await postJson('/api/diag/pair/begin', {duration_s: 120});
          showResult('Fenêtre de pairing ouverte (120s)');
        } catch (error) {
          showResult(error.message, false);
        } finally {
          btn.disabled = false;
        }
      });
    });
    document.querySelectorAll('[data-action="pair-end"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        btn.disabled = true;
        try {
          await postJson('/api/diag/pair/end');
          showResult('Fenêtre de pairing fermée');
        } catch (error) {
          showResult(error.message, false);
        } finally {
          btn.disabled = false;
        }
      });
    });
    document.querySelectorAll('[data-action="reset-backlog"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        if (!confirm('Émettre un reset backlog ?')) return;
        btn.disabled = true;
        try {
          await postJson('/api/diag/reset-backlog');
          showResult('Reset backlog envoyé');
        } catch (error) {
          showResult(error.message, false);
        } finally {
          btn.disabled = false;
        }
      });
    });
    document.querySelectorAll('[data-action="ping"]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        const asset = btn.dataset.asset;
        const mac = btn.dataset.mac;
        btn.disabled = true;
        try {
          const res = await postJson('/api/diag/ping', {asset_id: asset, mac});
          showResult(`Ping ${asset} corr=${res.correlation_id}`);
        } catch (error) {
          showResult(error.message, false);
        } finally {
          btn.disabled = false;
        }
      });
    });
  };

  attachCommandForm();
  attachRelayForm();
  attachBufferActions();
  attachDiagActions();
})();
