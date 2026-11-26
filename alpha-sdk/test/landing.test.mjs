import { buildSite } from '../scripts/make-docs.mjs';
import { test } from 'node:test';
import assert from 'node:assert/strict';

test('landing keeps the guide and includes six real captures with resolvable local links', () => {
  const files = buildSite();
  assert.match(files['index.html'], /landing\.css/);
  assert.match(files['getting-started.html'], /id="2-create-a-project"/);
  // External links keep their .md (GitHub renders it); only this site's pages become .html.
  assert.match(files['getting-started.html'], /href="https:\/\/github\.com\/[^"]+\/BUILDING\.md"/);
  assert.match(files['getting-started.html'], /href="first-game\.html"/);
  for (const name of ['flappy', 'platformer', 'topdown', 'puzzle', 'shmup', 'shaders']) {
    assert.ok(Buffer.isBuffer(files[`screenshot-${name}.png`]), name);
    assert.match(files['index.html'], new RegExp(`screenshot-${name}\\.png`));
  }
  for (const match of files['index.html'].matchAll(/(?:href|src)="([^"#]+)(?:#[^"]*)?"/g)) {
    if (/^(https?:|mailto:)/.test(match[1])) continue;
    assert.ok(match[1] in files, `missing landing target: ${match[1]}`);
  }
});


test('hero exposes seven accessible slides, a muted looping capture first, and the project logo', () => {
  const files = buildSite();
  const html = files['index.html'];
  const hero = html.split('id="example-slider"')[1]?.split('class="engine-strip"')[0] ?? '';
  assert.equal((hero.match(/data-slide=/g) ?? []).length, 7);
  assert.equal((hero.match(/data-select-slide=/g) ?? []).length, 7);
  const first = hero.split('data-slide=')[1];
  assert.match(first, /<video [^>]*autoplay muted loop playsinline/);
  assert.ok(Buffer.isBuffer(files['showcase-survivors.mp4']));
  assert.match(hero, /aria-roledescription="carousel"/);
  assert.match(hero, /data-slider-next/);
  assert.match(hero, /data-slider-prev/);
  assert.match(html.split('</header>')[0], /src="logo.png"/);
  assert.ok(Buffer.isBuffer(files['logo.png']));
});
