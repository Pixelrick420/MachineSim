'use strict';

(function () {
  const $ = (id) => document.getElementById(id);
  const machine = JSON.parse($('machine-data').textContent);
  const engine = new TuringEngine(machine, machine.input);
  const blank = machine.blank;

  const els = {
    viewport: $('viewport'), track: $('track'), headState: $('head-state'), last: $('last'),
    state: $('stat-state'), step: $('stat-step'), read: $('stat-read'), status: $('stat-status'),
    reset: $('btn-reset'), back: $('btn-back'), stepBtn: $('btn-step'), play: $('btn-play'), playLabel: $('play-label'),
    speed: $('speed'), speedOut: $('speed-out'), table: $('table'),
    form: $('input-form'), input: $('input'), error: $('input-error'),
    contents: $('contents'), contentsNote: $('contents-note'), meta: $('meta'), theme: $('theme'),
  };

  // ---------- tape ----------
  let cw = 56;
  let half = 12;
  let inputLen = engine.input.length;
  const cells = new Map();

  function measure() {
    cw = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--cw'), 10) || 56;
    half = Math.ceil(els.viewport.clientWidth / 2 / cw) + 2;
  }

  function setSymbol(el, ch) {
    if (el._ch === ch) return;
    el._ch = ch;
    el.firstChild.textContent = ch;
    el.firstChild.className = ch === blank ? (ch === '_' ? 'sym blank low' : 'sym blank') : 'sym';
  }

  function makeCell(i) {
    const el = document.createElement('div');
    el.className = i >= 0 && i < inputLen ? 'cell in' : 'cell';
    el.style.left = i * cw + 'px';
    const sym = document.createElement('span');
    const idx = document.createElement('span');
    idx.className = 'idx';
    idx.textContent = i;
    el.append(sym, idx);
    el._ch = null;
    return el;
  }

  function clearTape() {
    cells.clear();
    els.track.textContent = '';
  }

  function renderTape(slideMs, delayMs) {
    const head = engine.head;
    for (const [i, el] of cells) {
      if (i < head - half || i > head + half) { el.remove(); cells.delete(i); }
    }
    for (let i = head - half; i <= head + half; i++) {
      let el = cells.get(i);
      if (!el) { el = makeCell(i); cells.set(i, el); els.track.appendChild(el); }
      setSymbol(el, engine.read(i));
    }
    els.track.style.transition = slideMs > 0
      ? `transform ${slideMs}ms cubic-bezier(0.4, 0, 0.2, 1) ${delayMs}ms` : 'none';
    els.track.style.transform = `translateX(${-(head * cw + cw / 2)}px)`;
  }

  function flashWrite(pos) {
    const el = cells.get(pos);
    if (!el) return;
    el.classList.remove('wrote');
    void el.offsetWidth;
    el.classList.add('wrote');
  }

  // ---------- table ----------
  const tdByKey = new Map();
  const rowByState = new Map();
  let curTd = null, curRow = null;

  function buildTable() {
    const head = document.createElement('thead');
    const hr = document.createElement('tr');
    hr.appendChild(document.createElement('th'));
    engine.symbols.forEach((s) => {
      const th = document.createElement('th');
      th.textContent = s;
      hr.appendChild(th);
    });
    head.appendChild(hr);
    const body = document.createElement('tbody');
    engine.stateNames.forEach((name, si) => {
      const tr = document.createElement('tr');
      const th = document.createElement('th');
      th.scope = 'row';
      if (si === engine.startState) {
        const mark = document.createElement('span');
        mark.className = 'start-mark';
        mark.title = 'start state';
        th.appendChild(mark);
      }
      th.appendChild(document.createTextNode(name));
      tr.appendChild(th);
      engine.symbols.forEach((_, ci) => {
        const k = si * engine.S + ci;
        const td = document.createElement('td');
        const r = engine.rules[k];
        if (!r) {
          td.className = 'empty';
          td.textContent = '-';
        } else {
          td.appendChild(document.createTextNode(`${r.write} ${r.move[0]} `));
          if (r.next === 'HALT') {
            const h = document.createElement('span');
            h.className = 'halt';
            h.textContent = 'HALT';
            td.appendChild(h);
          } else {
            td.appendChild(document.createTextNode(r.next));
          }
        }
        tdByKey.set(k, td);
        tr.appendChild(td);
      });
      rowByState.set(si, tr);
      body.appendChild(tr);
    });
    els.table.append(head, body);
  }

  function highlightRule() {
    if (curTd) curTd.classList.remove('cur');
    if (curRow) curRow.classList.remove('cur-row');
    curTd = curRow = null;
    if (engine.halted) return;
    curRow = rowByState.get(engine.state);
    curRow.classList.add('cur-row');
    curTd = tdByKey.get(engine.currentKey);
    curTd.classList.add('cur');
  }

  // ---------- readouts ----------
  const STATUS = {
    ready: 'Ready', active: 'Paused', halted: 'Halted', stuck: 'Stuck', limit: 'Tape limit',
  };

  function fmtRule(t) {
    const r = t.rule;
    return `\u03b4(${t.state}, ${t.read}) = (${r.next}, ${r.write}, ${r.move[0]})`;
  }

  function renderReadouts() {
    const status = engine.status;
    els.state.textContent = engine.stateName;
    els.headState.textContent = engine.stateName;
    els.step.textContent = engine.steps.toLocaleString();
    els.read.textContent = engine.read(engine.head);
    const label = playing && status === 'active' ? 'Running' : STATUS[status];
    els.status.textContent = label;
    els.status.className = 'badge ' + (playing && status === 'active' ? 'active' : status);

    els.last.classList.toggle('bad', status === 'stuck' || status === 'limit');
    if (status === 'stuck') {
      els.last.textContent = `No rule for state ${engine.stateName} reading ${engine.read(engine.head)}`;
    } else if (status === 'limit') {
      els.last.textContent = 'Tape size limit reached';
    } else {
      const t = engine.lastTransition();
      els.last.textContent = t ? fmtRule(t) : 'Press play or step to begin';
    }

    const text = engine.contents(160);
    els.contents.textContent = '';
    if (text === null) {
      els.contentsNote.textContent = `${engine.nonBlank.toLocaleString()} non-blank cells`;
    } else {
      for (const ch of text) {
        const s = document.createElement('span');
        if (ch === blank) s.className = 'blank';
        s.textContent = ch;
        els.contents.appendChild(s);
      }
      els.contentsNote.textContent = text ? `${engine.nonBlank.toLocaleString()} non-blank cells` : 'Tape is empty';
    }

    const done = engine.finished;
    els.stepBtn.disabled = done;
    els.play.disabled = done;
    els.back.disabled = engine.histLen === 0;
    els.playLabel.textContent = playing ? 'Pause' : 'Play';
    els.play.dataset.playing = String(playing);
  }

  function render(slideMs, delayMs) {
    renderTape(slideMs, delayMs);
    highlightRule();
    renderReadouts();
  }

  // ---------- playback ----------
  let playing = false;
  let rate = 5;
  let acc = 0;
  let lastT = 0;
  let raf = 0;

  function readSpeed() {
    const v = +els.speed.value;
    rate = v >= 100 ? Infinity : 0.5 * Math.pow(10, v * 0.0335);
    if (rate === Infinity) els.speedOut.textContent = 'max';
    else if (rate < 10) els.speedOut.textContent = rate.toFixed(1) + ' steps/s';
    else if (rate < 1000) els.speedOut.textContent = Math.round(rate) + ' steps/s';
    else els.speedOut.textContent = (rate / 1000).toFixed(1) + 'k steps/s';
  }

  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  /** Run `n` steps, animating only when single steps are slow enough to see. */
  function advance(n) {
    const r = Math.min(rate, 1e9);
    const done = engine.run(n);
    if (done === 0) { render(0, 0); return; }
    if (done === 1 && r <= 60 && !reduceMotion) {
      const interval = 1000 / r;
      const slide = r <= 12 ? Math.min(380, Math.max(80, interval * 0.7)) : interval * 0.9;
      const delay = r <= 3 ? 140 : 0;
      const t = engine.lastTransition();
      render(slide, delay);
      if (t && t.read !== t.rule.write) flashWrite(t.pos);
    } else {
      render(0, 0);
    }
  }

  function frame(now) {
    if (!playing) return;
    const dt = Math.min(100, now - lastT);
    lastT = now;
    if (rate === Infinity) {
      const t0 = performance.now();
      let total = 0;
      while (performance.now() - t0 < 12 && !engine.finished) total += engine.run(20000);
      if (total) render(0, 0);
    } else {
      acc += (dt * rate) / 1000;
      const n = Math.min(Math.floor(acc), 2e6);
      if (n > 0) { acc -= n; advance(n); }
    }
    if (engine.finished) { pause(); return; }
    raf = requestAnimationFrame(frame);
  }

  function play() {
    if (playing || engine.finished) return;
    playing = true;
    acc = 1;
    lastT = performance.now();
    renderReadouts();
    raf = requestAnimationFrame(frame);
  }

  function pause() {
    playing = false;
    cancelAnimationFrame(raf);
    renderReadouts();
  }

  function stepOnce() {
    pause();
    if (engine.finished) return;
    const saved = rate;
    rate = 4;
    advance(1);
    rate = saved;
  }

  function stepBack() {
    pause();
    if (engine.back()) render(reduceMotion ? 0 : 220, 0);
  }

  function resetMachine(input) {
    pause();
    if (input !== undefined) {
      engine.reset(input);
      inputLen = input.length;
    } else {
      engine.reset(engine.input);
    }
    clearTape();
    render(0, 0);
  }

  // ---------- wiring ----------
  els.play.addEventListener('click', () => (playing ? pause() : play()));
  els.stepBtn.addEventListener('click', stepOnce);
  els.back.addEventListener('click', stepBack);
  els.reset.addEventListener('click', () => resetMachine());
  els.speed.addEventListener('input', readSpeed);

  els.form.addEventListener('submit', (e) => {
    e.preventDefault();
    const value = els.input.value.trim();
    const bad = engine.invalidSymbol(value);
    if (bad !== null) {
      els.error.textContent = `Symbol "${bad}" is not in the machine's alphabet.`;
      return;
    }
    els.error.textContent = '';
    resetMachine(value);
  });
  els.input.addEventListener('input', () => { els.error.textContent = ''; });

  document.addEventListener('keydown', (e) => {
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'BUTTON' || e.ctrlKey || e.metaKey || e.altKey) return;
    if (e.key === ' ') { e.preventDefault(); playing ? pause() : play(); }
    else if (e.key === 'ArrowRight') { e.preventDefault(); stepOnce(); }
    else if (e.key === 'ArrowLeft') { e.preventDefault(); stepBack(); }
    else if (e.key === 'r' || e.key === 'R') resetMachine();
  });

  let resizeTimer = 0;
  window.addEventListener('resize', () => {
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => { measure(); clearTape(); render(0, 0); }, 100);
  });

  els.theme.addEventListener('click', () => {
    const root = document.documentElement;
    const dark = root.dataset.theme
      ? root.dataset.theme === 'dark'
      : window.matchMedia('(prefers-color-scheme: dark)').matches;
    root.dataset.theme = dark ? 'light' : 'dark';
    try { localStorage.setItem('turing-theme', root.dataset.theme); } catch (e) { /* storage unavailable */ }
  });
  try {
    const saved = localStorage.getItem('turing-theme');
    if (saved === 'light' || saved === 'dark') document.documentElement.dataset.theme = saved;
  } catch (e) { /* storage unavailable */ }

  // ---------- init ----------
  function buildMeta() {
    const rows = [
      ['Blank', blank],
      ['Alphabet', machine.alphabet.join(' ')],
      ['Start', machine.start],
      ['States', String(machine.states.length)],
    ];
    rows.forEach(([k, v]) => {
      const dt = document.createElement('dt');
      const dd = document.createElement('dd');
      dt.textContent = k;
      dd.textContent = v;
      els.meta.append(dt, dd);
    });
  }

  els.input.value = machine.input;
  buildTable();
  buildMeta();
  readSpeed();
  measure();
  render(0, 0);
})();
