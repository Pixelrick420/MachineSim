'use strict';
// Static lint of the generated page's CSS and markup. It guards the interface
// rules the page was built to: no `transition: all`, hover only on pointers
// that can hover, movement gated behind prefers-reduced-motion, and icons that
// follow the text color. It reads the compiled output, so it needs no browser.

const { spawnSync } = require('child_process');
const path = require('path');

const root = path.join(__dirname, '..');
const compiler = path.join(root, 'bin', 'turing-compiler');
let failed = 0;

function check(name, ok, detail) {
  console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${ok || !detail ? '' : '  (' + detail + ')'}`);
  if (!ok) failed++;
}

/** Remove every block whose header matches `header`, braces balanced. */
function stripBlocks(css, header) {
  const re = new RegExp(header.source, 'g');
  let out = '';
  let last = 0;
  let m;
  while ((m = re.exec(css))) {
    let depth = 0;
    let j = css.indexOf('{', m.index);
    for (; j < css.length; j++) {
      if (css[j] === '{') depth++;
      else if (css[j] === '}' && --depth === 0) break;
    }
    out += css.slice(last, m.index);
    last = j + 1;
    re.lastIndex = last;
  }
  return out + css.slice(last);
}

// Lints an example by default, or any compiled page given as the first argument.
let html;
if (process.argv[2]) {
  html = require('fs').readFileSync(process.argv[2], 'utf8');
} else {
  const res = spawnSync(compiler, [path.join(root, 'examples', 'unary-add.machine')], { encoding: 'utf8', maxBuffer: 1 << 26 });
  if (res.status !== 0) throw new Error(res.stderr);
  html = res.stdout;
}
const css = html.match(/<style>([\s\S]*?)<\/style>/)[1];

const HOVER = /@media\s*\(hover:\s*hover\)/;
const NO_PREF = /@media\s*\(prefers-reduced-motion:\s*no-preference\)/;
const KEYFRAMES = /@keyframes\s+[\w-]+/;

check('no transition on all properties', !/transition(-property)?:\s*all\b/.test(css) && !/\btransition:[^;]*\ball\b/.test(css));
check('no will-change: all', !/will-change:\s*all\b/.test(css));

const outsideHover = stripBlocks(css, HOVER);
check('hover styles only inside @media (hover: hover)', !/:hover/.test(outsideHover), (outsideHover.match(/[^{}]*:hover[^{]*/) || [''])[0].trim());

// Movement, scale and blur only run when the visitor has not asked for less motion.
const outsideMotion = stripBlocks(stripBlocks(css, NO_PREF), KEYFRAMES);
check('no scale, translate or blur outside no-preference', !/(^|[;{\s])(scale|translate):/.test(outsideMotion) && !/blur\(/.test(outsideMotion));

const moving = [...css.matchAll(/@keyframes\s+([\w-]+)\s*\{((?:[^{}]|\{[^{}]*\})*)\}/g)]
  .filter((m) => /translate|scale|rotate|transform|blur/.test(m[2]))
  .map((m) => m[1]);
const leaking = moving.filter((name) => new RegExp(`(^|[^\\w-])${name}([^\\w-]|$)`).test(outsideMotion));
check('keyframes that move are only used under no-preference', moving.length > 0 && leaking.length === 0, leaking.join(', '));

check('icons are drawn with currentColor, never a hard coded color', !/<svg[^>]*(fill|stroke)="#/.test(html) && !/<(path|circle|rect)[^>]*(fill|stroke)="#/.test(html));
check('pressed buttons scale to 0.96', /:active[^{]*\{[^}]*scale:\s*0\.96/.test(css));
check('theme switch suppresses transitions', /transition:none !important/.test(html));

console.log(failed ? `\n${failed} check(s) failed` : '\nall ui checks passed');
process.exit(failed ? 1 : 0);
