"use strict";

class TuringEngine {
  constructor(machine, input) {
    this.machine = machine;
    this.HISTORY = 1 << 20;
    this.MAX_TAPE = 1 << 26;

    const symbols = [machine.blank];
    const addSymbol = (c) => {
      if (!symbols.includes(c)) symbols.push(c);
    };
    machine.alphabet.forEach(addSymbol);
    machine.states.forEach((s) =>
      s.rules.forEach((r) => {
        addSymbol(r.read);
        addSymbol(r.write);
      }),
    );
    this.symbols = symbols;
    this.symIndex = new Map(symbols.map((c, i) => [c, i]));

    this.stateNames = machine.states.map((s) => s.name);
    this.HALT = this.stateNames.length;
    this.stateIndex = new Map(this.stateNames.map((n, i) => [n, i]));
    this.startState = this.stateIndex.get(machine.start);

    const S = symbols.length;
    this.S = S;
    const size = this.HALT * S;
    this.tNext = new Int32Array(size).fill(-1);
    this.tWrite = new Uint8Array(size);
    this.tMove = new Int8Array(size);
    this.rules = new Array(size).fill(null);

    const moves = { LEFT: -1, STAY: 0, RIGHT: 1 };
    machine.states.forEach((state, si) => {
      state.rules.forEach((r) => {
        const k = si * S + this.symIndex.get(r.read);
        this.tNext[k] =
          r.next === "HALT" ? this.HALT : this.stateIndex.get(r.next);
        this.tWrite[k] = this.symIndex.get(r.write);
        this.tMove[k] = moves[r.move];
        this.rules[k] = r;
      });
    });

    this.hHead = new Int32Array(this.HISTORY);
    this.hState = new Int32Array(this.HISTORY);
    this.hSym = new Uint8Array(this.HISTORY);

    this.reset(input === undefined ? machine.input : input);
  }

  invalidSymbol(input) {
    for (const c of input) if (!this.symIndex.has(c)) return c;
    return null;
  }

  reset(input) {
    this.input = input;
    this.cap = Math.max(1024, 1 << Math.ceil(Math.log2(input.length * 2 + 8)));
    this.origin = this.cap >> 1;
    this.tape = new Uint8Array(this.cap);
    this.nonBlank = 0;
    this.lo = 0;
    this.hi = -1;
    for (let i = 0; i < input.length; i++) {
      const s = this.symIndex.get(input[i]);
      this.tape[this.origin + i] = s;
      if (s !== 0) {
        this.nonBlank++;
        if (this.hi < this.lo) {
          this.lo = i;
          this.hi = i;
        } else this.hi = i;
      }
    }
    this.head = 0;
    this.state = this.startState;
    this.steps = 0;
    this.histPos = 0;
    this.histLen = 0;
    this.limit = false;
  }

  _grow() {
    const newCap = this.cap * 2;
    if (newCap > this.MAX_TAPE) return false;
    const next = new Uint8Array(newCap);
    const shift = (newCap - this.cap) >> 1;
    next.set(this.tape, shift);
    this.tape = next;
    this.origin += shift;
    this.cap = newCap;
    return true;
  }

  get halted() {
    return this.state === this.HALT;
  }
  get stuck() {
    return this.state !== this.HALT && this.tNext[this.currentKey] < 0;
  }
  get finished() {
    return this.halted || this.limit || this.stuck;
  }
  get currentKey() {
    return this.state * this.S + this.readIndex(this.head);
  }
  get stateName() {
    return this.halted ? "HALT" : this.stateNames[this.state];
  }

  get status() {
    if (this.halted) return "halted";
    if (this.limit) return "limit";
    if (this.stuck) return "stuck";
    return this.steps === 0 ? "ready" : "active";
  }

  readIndex(pos) {
    const i = pos + this.origin;
    return i >= 0 && i < this.cap ? this.tape[i] : 0;
  }

  read(pos) {
    return this.symbols[this.readIndex(pos)];
  }

  /** Run up to `max` transitions. Returns how many were executed. */
  run(max) {
    const S = this.S,
      HALT = this.HALT;
    const { tNext, tWrite, tMove, hHead, hState, hSym } = this;
    const MASK = this.HISTORY - 1;
    let tape = this.tape,
      origin = this.origin,
      cap = this.cap;
    let head = this.head,
      state = this.state,
      steps = this.steps;
    let hp = this.histPos,
      hl = this.histLen,
      nb = this.nonBlank;
    let lo = this.lo,
      hi = this.hi;
    let n = 0;

    while (n < max && state !== HALT) {
      const idx = head + origin;
      const sym = tape[idx];
      const k = state * S + sym;
      const next = tNext[k];
      if (next < 0) break;

      hHead[hp] = head;
      hState[hp] = state;
      hSym[hp] = sym;
      hp = (hp + 1) & MASK;
      if (hl < MASK + 1) hl++;

      const w = tWrite[k];
      if (w !== sym) {
        tape[idx] = w;
        if (sym === 0) nb++;
        else if (w === 0) nb--;
        if (w !== 0) {
          if (hi < lo) {
            lo = head;
            hi = head;
          } else if (head < lo) lo = head;
          else if (head > hi) hi = head;
        }
      }
      state = next;
      head += tMove[k];
      steps++;
      n++;

      const i2 = head + origin;
      if (i2 < 2 || i2 >= cap - 2) {
        this.tape = tape;
        this.origin = origin;
        this.cap = cap;
        if (!this._grow()) {
          this.limit = true;
          break;
        }
        tape = this.tape;
        origin = this.origin;
        cap = this.cap;
      }
    }

    this.head = head;
    this.state = state;
    this.steps = steps;
    this.histPos = hp;
    this.histLen = hl;
    this.nonBlank = nb;
    this.lo = lo;
    this.hi = hi;
    return n;
  }

  step() {
    return this.run(1) === 1;
  }

  /** Undo one transition. Returns false if history is exhausted. */
  back() {
    if (this.histLen === 0) return false;
    const MASK = this.HISTORY - 1;
    this.histPos = (this.histPos - 1) & MASK;
    this.histLen--;
    const head = this.hHead[this.histPos];
    const sym = this.hSym[this.histPos];
    const idx = head + this.origin;
    const cur = this.tape[idx];
    if (cur !== sym) {
      if (cur === 0) this.nonBlank++;
      else if (sym === 0) this.nonBlank--;
      this.tape[idx] = sym;
    }
    this.head = head;
    this.state = this.hState[this.histPos];
    this.steps--;
    this.limit = false;
    return true;
  }

  /** The most recently executed transition, derived from history. */
  lastTransition() {
    if (this.histLen === 0) return null;
    const p = (this.histPos - 1) & (this.HISTORY - 1);
    const key = this.hState[p] * this.S + this.hSym[p];
    return {
      pos: this.hHead[p],
      key,
      rule: this.rules[key],
      state: this.stateNames[this.hState[p]],
      read: this.symbols[this.hSym[p]],
    };
  }

  /** Non-blank span of the tape as a string, or null if longer than maxLen. */
  contents(maxLen) {
    if (this.hi < this.lo || this.nonBlank === 0) return "";
    if (this.hi - this.lo + 1 > maxLen) return null;
    let out = "";
    for (let p = this.lo; p <= this.hi; p++) out += this.read(p);
    return out;
  }
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { TuringEngine };
}
