// Turing Machine Simulation
// Generated from Turing Machine specification

const turingMachine = {
  blank: '_',
  alphabet: ['1', '+', '=', '_'],
  initialState: 'findPlus',
  input: '111+11=',
  states: {
    'findPlus': {
      '1': { write: '1', next: 'findPlus', move: 'RIGHT' },
      '+': { write: '1', next: 'findEqual', move: 'RIGHT' },
      '_': { write: '_', next: 'HALT', move: 'RIGHT' }
    },
    'findEqual': {
      '1': { write: '1', next: 'findEqual', move: 'RIGHT' },
      '=': { write: '_', next: 'cleanup', move: 'LEFT' },
      '_': { write: '_', next: 'HALT', move: 'RIGHT' }
    },
    'cleanup': {
      '1': { write: '_', next: 'cleanup', move: 'LEFT' },
      '+': { write: '_', next: 'HALT', move: 'RIGHT' },
      '_': { write: '_', next: 'HALT', move: 'RIGHT' }
    }
  }
};

// Turing Machine Visualizer - barebones working version
class TuringVisualizer {
  constructor(machine, containerId = 'turing-viz') {
    this.machine = machine;
    this.container = document.getElementById(containerId);
    this.tape = [];
    this.head = 0;
    this.state = machine.initialState;
    this.stepCount = 0;
    this.running = false;
    this.speed = 500;
    this.history = [];
    this.cellElements = [];
    this.timerId = null;
    this.init();
  }

  init() {
    console.log('Initializing machine:', this.machine.initialState, this.machine.input);
    const inputChars = this.machine.input.split('');
    const blank = this.machine.blank;
    const PAD = 30;
    this.tape = [];
    for (let i = 0; i < PAD; i++) this.tape.push(blank);
    for (const c of inputChars) this.tape.push(c);
    for (let i = 0; i < PAD; i++) this.tape.push(blank);
    this.head = PAD;
    this.stepCount = 0;
    this.history = [];
    this.render();
  }

  step() {
    if (this.running) this.stop(); // Stop auto-run if stepping manually
    if (this.state === 'HALT') {
      console.log('Already halted');
      return false;
    }
    const currentSymbol = this.tape[this.head] || this.machine.blank;
    const stateDef = this.machine.states[this.state];
    if (!stateDef || !stateDef[currentSymbol]) {
      console.log('No transition for state', this.state, 'symbol', currentSymbol);
      return false;
    }
    const transition = stateDef[currentSymbol];
    console.log('Step', this.stepCount, ': state=', this.state, 'read=', currentSymbol, '-> write=', transition.write, 'next=', transition.next, 'move=', transition.move);
    this.history.push({ tape: [...this.tape], head: this.head, state: this.state, step: this.stepCount });
    this.tape[this.head] = transition.write;
    this.state = transition.next;
    if (transition.move === 'LEFT') {
      this.head--;
      if (this.head < 0) {
        this.tape.unshift(this.machine.blank);
        this.head = 0;
      }
    } else {
      this.head++;
      if (this.head >= this.tape.length) {
        this.tape.push(this.machine.blank);
      }
    }
    this.stepCount++;
    this.render();
    return this.state !== 'HALT';
  }

  run() {
    if (this.running) return;
    this.stop(); // Clear any existing timer
    this.running = true;
    const doStep = () => {
      if (!this.running || this.state === 'HALT') {
        this.running = false;
        this.timerId = null;
        this.updateControls();
        return;
      }
      this.step();
      if (this.running) {
        this.timerId = setTimeout(doStep, this.speed);
      }
    }
    doStep();
    this.updateControls();
  }

  stop() {
    this.running = false;
    if (this.timerId) {
      clearTimeout(this.timerId);
      this.timerId = null;
    }
    this.updateControls();
  }

  reset() {
    this.stop();
    this.init();
  }

  undo() {
    if (this.history.length === 0) return;
    const prev = this.history.pop();
    this.tape = prev.tape;
    this.head = prev.head;
    this.state = prev.state;
    this.stepCount = prev.step;
    this.render();
  }

  setSpeed(ms) { this.speed = parseInt(ms, 10); }

  render() {
    if (!this.container) return;
    const VISIBLE = 41;
    const half = Math.floor(VISIBLE / 2);
    let start = this.head - half;
    if (start < 0) start = 0;
    if (start + VISIBLE > this.tape.length) start = this.tape.length - VISIBLE;
    if (start < 0) start = 0;
    const end = Math.min(start + VISIBLE, this.tape.length);
    
    const tapeDiv = this.container.querySelector('.tape');
    if (tapeDiv) {
      tapeDiv.innerHTML = '';
    } else {
      this.container.innerHTML = '';
      this.buildUI();
      return;
    }
    this.cellElements = [];
    for (let i = start; i < end; i++) {
      const cell = document.createElement('div');
      cell.className = 'cell';
      cell.dataset.index = i;
      if (i === this.head) cell.classList.add('head');
      if (i >= 30 && i < 30 + this.machine.input.length && this.stepCount === 0) {
        cell.classList.add('input');
      }
      const sym = document.createElement('div');
      sym.className = 'symbol';
      sym.textContent = this.tape[i] || this.machine.blank;
      if (i === this.head) {
        const marker = document.createElement('div');
        marker.className = 'head-marker';
        marker.textContent = '\u25BC';
        cell.appendChild(marker);
      }
      cell.appendChild(sym);
      tapeDiv.appendChild(cell);
      this.cellElements.push(cell);
    }
    this.updateStateDisplay();
    this.updateControls();
  }

  buildUI() {
    this.container.innerHTML = '';
    // Top bar with state and controls
    const topBar = document.createElement('div');
    topBar.className = 'top-bar';
    this.stateDiv = document.createElement('div');
    this.stateDiv.className = 'state-display';
    topBar.appendChild(this.stateDiv);
    const controls = document.createElement('div');
    controls.className = 'controls';
    controls.innerHTML = ''
      + '<button id="btn-step">Step</button>'
      + '<button id="btn-run">Run</button>'
      + '<button id="btn-stop">Stop</button>'
      + '<button id="btn-undo">Undo</button>'
      + '<button id="btn-reset">Reset</button>'
      + '<label>Speed: <input type="range" id="speed" min="50" max="2000" value="500"></label>'
    topBar.appendChild(controls);
    this.container.appendChild(topBar);
    // Tape area
    const tapeWrap = document.createElement('div');
    tapeWrap.className = 'tape-wrapper';
    const tapeDiv = document.createElement('div');
    tapeDiv.className = 'tape';
    tapeWrap.appendChild(tapeDiv);
    this.container.appendChild(tapeWrap);
    // Bind events
    document.getElementById('btn-step').onclick = () => this.step();
    document.getElementById('btn-run').onclick = () => this.run();
    document.getElementById('btn-stop').onclick = () => this.stop();
    document.getElementById('btn-undo').onclick = () => this.undo();
    document.getElementById('btn-reset').onclick = () => this.reset();
    document.getElementById('speed').oninput = (e) => this.setSpeed(e.target.value);
    this.render();
  }

  updateStateDisplay() {
    if (!this.stateDiv) return;
    this.stateDiv.textContent = 'State: ' + this.state + ' | Step: ' + this.stepCount;
  }

  updateControls() {
    const halted = this.state === 'HALT';
    document.getElementById('btn-step').disabled = halted || this.running;
    document.getElementById('btn-run').disabled = halted || this.running;
    document.getElementById('btn-stop').disabled = !this.running;
    document.getElementById('btn-undo').disabled = this.history.length === 0;
  }
}

let viz;
function initViz() { viz = new TuringVisualizer(turingMachine); }
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initViz);
} else {
  initViz();
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { turingMachine, TuringVisualizer };
}
