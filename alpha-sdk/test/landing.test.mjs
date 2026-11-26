import { buildSite } from '../scripts/make-docs.mjs';
import { test } from 'node:test';
import assert from 'node:assert/strict';

test('landing keeps the guide and includes five real captures with resolvable local links', () => {
  const files = buildSite();
  assert.match(files['index.html'], /landing\.css/);
  assert.match(files['getting-started.html'], /id="make-a-game"/);
  for (const name of ['flappy', 'platformer', 'topdown', 'puzzle', 'shmup']) {
    assert.ok(Buffer.isBuffer(files[`screenshot-${name}.png`]), name);
    assert.match(files['index.html'], new RegExp(`screenshot-${name}\\.png`));
  }
  for (const match of files['index.html'].matchAll(/(?:href|src)="([^"#]+)(?:#[^"]*)?"/g)) {
    if (/^(https?:|mailto:)/.test(match[1])) continue;
    assert.ok(match[1] in files, `missing landing target: ${match[1]}`);
  }
});
