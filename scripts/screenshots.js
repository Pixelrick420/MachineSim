'use strict';
// Regenerates the README screenshots from the current templates.
//
//   make
//   npm install --no-save playwright && npx playwright install chromium
//   node scripts/screenshots.js
//
// Playwright is only needed for this script, it is not a dependency of the
// project. Output goes to docs/screenshot-light.png and docs/screenshot-dark.png.

const { spawnSync } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

let chromium;
try {
  ({ chromium } = require('playwright'));
} catch (e) {
  console.error('playwright is not installed: npm install --no-save playwright');
  process.exit(1);
}

const root = path.join(__dirname, '..');
const machine = process.env.MACHINE || path.join(root, 'examples', 'unary-add.machine');
const steps = Number(process.env.STEPS || 4);
const scale = Number(process.env.SCALE || 1.5);

const page_html = path.join(os.tmpdir(), 'machinesim-screenshot.html');
const built = spawnSync(path.join(root, 'bin', 'turing-compiler'), ['-o', page_html, machine], { encoding: 'utf8' });
if (built.status !== 0) {
  console.error(built.stderr || 'compile failed, run make first');
  process.exit(1);
}

(async () => {
  const browser = await chromium.launch();
  for (const scheme of ['light', 'dark']) {
    const context = await browser.newContext({
      viewport: { width: 1040, height: 800 },
      deviceScaleFactor: scale,
      colorScheme: scheme,
    });
    const page = await context.newPage();
    await page.goto('file://' + page_html);
    await page.waitForTimeout(1000); // let the entrance finish
    for (let i = 0; i < steps; i++) {
      await page.keyboard.press('ArrowRight');
      await page.waitForTimeout(450);
    }
    await page.waitForTimeout(250); // slide done, write highlight still fading
    const out = path.join(root, 'docs', `screenshot-${scheme}.png`);
    await page.screenshot({ path: out, fullPage: true });
    console.log(`${path.relative(root, out)}  ${(fs.statSync(out).size / 1024).toFixed(0)} KB`);
    await context.close();
  }
  await browser.close();
})();
