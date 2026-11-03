'use strict';
// Compiles every example and error case, then runs the generated machines
// headlessly through the same engine the page uses.

const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const { TuringEngine } = require('../templates/engine.js');

const root = path.join(__dirname, '..');
const compiler = path.join(root, 'bin', 'turing-compiler');
let failed = 0;

function check(name, ok, detail) {
  console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${ok || !detail ? '' : '  (' + detail + ')'}`);
  if (!ok) failed++;
}

function compile(file) {
  return spawnSync(compiler, [file], { encoding: 'utf8', maxBuffer: 1 << 26 });
}

function load(file) {
  const res = compile(file);
  if (res.status !== 0) throw new Error(res.stderr);
  const json = res.stdout.match(/<script id="machine-data" type="application\/json">([\s\S]*?)<\/script>/);
  return { html: res.stdout, machine: JSON.parse(json[1]) };
}

function run(machine, input) {
  const e = new TuringEngine(machine, input);
  while (!e.finished) e.run(1e6);
  return e;
}

// Every example compiles to a single file with nothing left to fetch.
for (const f of fs.readdirSync(path.join(root, 'examples')).filter((f) => f.endsWith('.machine'))) {
  const { html } = load(path.join(root, 'examples', f));
  const external = /<(script|link|img)\b[^>]*\b(src|href)=/i.test(html);
  check(`${f} compiles to a standalone page`, !external && !html.includes('{{') && !/gradient/i.test(html));
}

const ex = (n) => load(path.join(root, 'examples', n + '.machine')).machine;

let e = run(ex('binary-increment'));
check('binary-increment: 1011 -> 1100', e.halted && e.contents(99) === '1100', e.contents(99));

e = run(ex('unary-add'));
check('unary-add: 111+11= -> 11111', e.halted && e.nonBlank === 5 && e.contents(99).replace(/_/g, '') === '11111');

e = run(ex('busy-beaver'));
check('busy-beaver: 47176870 steps, 4098 ones', e.halted && e.steps === 47176870 && e.nonBlank === 4098, `${e.steps} ${e.nonBlank}`);

const pal = ex('palindrome');
for (const [input, accept] of [['', true], ['1', true], ['101', true], ['1001', true], ['11011', true], ['10', false], ['1101', false], ['0110', true], ['01', false]]) {
  e = run(pal, input);
  const out = e.read(-1);
  check(`palindrome: "${input}" ${accept ? 'accepted' : 'rejected'}`, e.halted && out === (accept ? '1' : '0'), `wrote ${out}`);
}

// Undo restores the exact earlier configuration.
e = new TuringEngine(ex('unary-add'));
const snap = () => JSON.stringify([e.head, e.state, e.steps, e.contents(99)]);
const before = [];
for (let i = 0; i < 5; i++) { before.push(snap()); e.step(); }
let ok = true;
for (let i = 4; i >= 0; i--) { e.back(); ok = ok && snap() === before[i]; }
check('back() restores previous configurations', ok && !e.back());

// A machine with a missing rule gets stuck instead of looping or crashing.
e = run({ name: 't', blank: '_', alphabet: ['_', '1'], start: 'a', input: '1', states: [{ name: 'a', rules: [{ read: '1', write: '1', next: 'a', move: 'RIGHT' }] }] });
check('missing rule reports stuck', e.status === 'stuck' && e.steps === 1);

// Compile errors are reported with a message and a non-zero status.
const errDir = path.join(__dirname, 'errors');
for (const f of fs.readdirSync(errDir).filter((f) => f.endsWith('.machine'))) {
  const file = path.join(errDir, f);
  const expect = fs.readFileSync(file, 'utf8').match(/expect: (.*)/)[1];
  const res = compile(file);
  check(`error: ${f}`, res.status !== 0 && res.stdout === '' && res.stderr.includes(expect), res.stderr.trim());
}

console.log(failed ? `\n${failed} check(s) failed` : '\nall checks passed');
process.exit(failed ? 1 : 0);
