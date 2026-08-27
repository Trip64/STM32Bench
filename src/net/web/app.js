let currentCategory = 'ALL';
let currentBenchmarks = [];

function filterCategory(cat) {
  currentCategory = cat;
  document.querySelectorAll('.tab').forEach(btn => {
    btn.classList.toggle('active', btn.textContent.trim().startsWith(cat) || (cat === 'ALL' && btn.textContent.trim().startsWith('All')));
  });
  filterTable();
}

function filterTable() {
  const query = (document.getElementById('search-box')?.value || '').toLowerCase();
  const rows = document.querySelectorAll('#bench-body tr');
  rows.forEach(row => {
    const rowCat = row.getAttribute('data-cat') || '';
    const rowText = row.textContent.toLowerCase();
    
    let matchesCat = false;
    if (currentCategory === 'ALL') matchesCat = true;
    else if (currentCategory === 'DSP') matchesCat = (rowCat === 'DSP' || rowCat === 'CORDIC' || rowCat === 'FMAC' || rowCat === 'Audio');
    else if (currentCategory === 'IO') matchesCat = (rowCat === 'IO' || rowCat === 'RealTime');
    else if (currentCategory === 'Crypto') matchesCat = (rowCat === 'Crypto' || rowCat === 'Compress');
    else matchesCat = (rowCat === currentCategory);

    const matchesQuery = !query || rowText.includes(query);
    row.style.display = (matchesCat && matchesQuery) ? '' : 'none';
  });
}

function runBenchmarks() {
  const btn = document.getElementById('btn-run');
  btn.disabled = true;
  btn.textContent = 'Running...';

  fetch('/run.cgi')
    .then(() => {
      setTimeout(() => {
        fetchLiveJSON();
        btn.disabled = false;
        btn.textContent = 'Run All';
      }, 1500);
    })
    .catch(() => {
      btn.disabled = false;
      btn.textContent = 'Run All';
    });
}

function fetchLiveJSON() {
  fetch('/api/benchmarks')
    .then(r => r.json())
    .then(data => {
      if (data.clock_mhz) {
        const el = document.getElementById('hdr-clk');
        if (el) el.textContent = data.clock_mhz + ' MHz';
      }
      if (data.stm32mark) {
        const elHdr = document.getElementById('hdr-mark');
        if (elHdr) elHdr.textContent = Number(data.stm32mark).toLocaleString();

        const elTotal = document.getElementById('sc-total');
        if (elTotal) elTotal.textContent = Number(data.stm32mark).toLocaleString();

        const elMult = document.getElementById('sc-mult');
        if (elMult) elMult.textContent = (data.stm32mark / 1000.0).toFixed(1) + 'x';
      }

      if (data.scores) {
        const cats = ['cpu', 'fpu', 'dsp', 'gfx', 'ai', 'crypto', 'io', 'mem'];
        cats.forEach(c => {
          const val = data.scores[c];
          if (val !== undefined) {
            const elVal = document.getElementById('sc-' + c);
            if (elVal) elVal.textContent = Number(val).toLocaleString();
            const elBar = document.getElementById('bar-' + c);
            if (elBar) elBar.style.width = Math.min(100, Math.max(5, Math.round((val / 10000.0) * 100))) + '%';
          }
        });
      }

      if (data.benchmarks && Array.isArray(data.benchmarks)) {
        currentBenchmarks = data.benchmarks;
        updateTable(data.benchmarks);
      }
    })
    .catch(() => {});
}

function updateTable(benchmarks) {
  const tbody = document.getElementById('bench-body');
  if (!tbody || !benchmarks.length) return;

  tbody.innerHTML = '';
  benchmarks.forEach(b => {
    const tr = document.createElement('tr');
    tr.setAttribute('data-cat', b.category);

    const hw = b.available ? 
      '<span class="tag-hw">HW</span>' : 
      '<span class="tag-sw">N/A</span>';

    tr.innerHTML = `
      <td><span class="cat c-${b.category}">${b.category}</span></td>
      <td><strong>${b.name}</strong></td>
      <td><span class="score">${b.score.toFixed(2)}</span></td>
      <td class="mono">${b.unit}</td>
      <td class="mono">${b.cycles.toLocaleString()}</td>
      <td class="mono">${b.time_us.toLocaleString()}</td>
      <td>${hw}</td>
    `;
    tbody.appendChild(tr);
  });

  filterTable();
}

document.addEventListener('DOMContentLoaded', () => {
  fetchLiveJSON();
});
