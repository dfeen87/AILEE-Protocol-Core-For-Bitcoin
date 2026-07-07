fetch('dashboard.json')
  .then(response => response.json())
  .then(data => {
      if (data && data.telemetry) {
          renderTickChart(data.telemetry);
          renderCoherenceChart(data.telemetry);
          renderStateRootChart(data.telemetry);
      }
  })
  .catch(err => {
      console.error('Error loading deterministic telemetry:', err);
      document.getElementById('tickChart').innerHTML = '<div class="error">Failed to load deterministic telemetry</div>';
  });

function renderTickChart(samples) {
    const ticks = samples.map(s => s.tick);
    const epochs = samples.map(s => s.epoch);

    const ctx = document.getElementById('tickChart');
    ctx.innerHTML = `
        <h3>Tick & Epoch Progression</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Ticks</span><span class="metric-value">[ ${ticks.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Epochs</span><span class="metric-value">[ ${epochs.join(', ')} ]</span></div>
        </div>
    `;
}

function renderCoherenceChart(samples) {
    const total = samples.map(s => s.total_nodes);
    const inSync = samples.map(s => s.in_sync_nodes);
    const scores = samples.map(s => s.global_coherence_score);

    const ctx = document.getElementById('coherenceChart');
    ctx.innerHTML = `
        <h3>Global Coherence</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Total Nodes</span><span class="metric-value">[ ${total.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">In-Sync Nodes</span><span class="metric-value">[ ${inSync.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Coherence Score</span><span class="metric-value">[ ${scores.join(', ')} ]</span></div>
        </div>
    `;
}

function renderStateRootChart(samples) {
    const consistent = samples.map(s => s.consistent_state_root_nodes);
    const inconsistent = samples.map(s => s.inconsistent_state_root_nodes);

    const ctx = document.getElementById('stateRootChart');
    ctx.innerHTML = `
        <h3>State-Root Consistency</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Consistent Nodes</span><span class="metric-value">[ ${consistent.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Inconsistent Nodes</span><span class="metric-value">[ ${inconsistent.join(', ')} ]</span></div>
        </div>
    `;
}
