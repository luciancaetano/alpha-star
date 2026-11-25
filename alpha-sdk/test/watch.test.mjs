import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { exportPack } from "../lib/pack.mjs";
import { watchProject } from "../lib/watch.mjs";

const source = (speed) => `
import { define } from "alpha";
interface T { actions: never; sounds: never; images: never; tags: never; scenes: { play: undefined }; ui: {} }
const { scene, game } = define<T>();
export const SPEED: number = ${speed};
export default game({ config: { title: "t", size: [10, 10], seed: 1, actions: {} },
  start: "play", scenes: { play: scene("play", {}) }, assets: { sounds: {} } });
`;

function project() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-watch-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), source(111));
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "dist") };
}

/** Starts watching; next() resolves with the result of the next export. */
function watching(p) {
  const results = [];
  const waiters = [];
  const w = watchProject({ ...p, debounceMs: 100, onExport: (r) => (waiters.shift() ?? ((x) => results.push(x)))(r) });
  const next = () => (results.length ? Promise.resolve(results.shift()) : new Promise((r) => waiters.push(r)));
  // Windows may report the fixture's own creation late (an extra export of the old source):
  // skip exports until one reflects what the test just wrote (at most a few).
  const until = async (seen) => {
    for (let i = 0; i < 4; i++) {
      const r = await next();
      if (seen(r)) return r;
    }
    throw new Error("no export reflected the change");
  };
  return { next, until, close: () => w.close() };
}

test("watch re-exports the pack when a source file changes; a type error leaves the pack untouched", async () => {
  const p = project();
  const w = watching(p);
  try {
    assert.ok((await exportPack(p)).ok);
    const main = path.join(p.out, "main.js");
    assert.match(fs.readFileSync(main, "utf8"), /111/);
    fs.writeFileSync(p.entry, source(222));
    const ok = await w.until((r) => !r.ok || /222/.test(fs.readFileSync(main, "utf8")));
    assert.deepEqual(ok.problems, []);
    assert.match(fs.readFileSync(main, "utf8"), /222/);
    const before = fs.readFileSync(main, "utf8");
    fs.writeFileSync(p.entry, source(`"fast"`));
    const bad = await w.until((r) => !r.ok);
    assert.equal(bad.ok, false);
    assert.match(bad.problems[0].message, /string.*number|number/);
    assert.equal(fs.readFileSync(main, "utf8"), before);
  } finally {
    w.close();
    fs.rmSync(p.root, { recursive: true, force: true });
  }
});

test("watch: a burst of saves exports once; the pack itself is not watched", async () => {
  const p = project();
  let exports = 0;
  const w = watchProject({ ...p, debounceMs: 150, onExport: () => exports++ });
  try {
    for (const speed of [1, 2, 3]) fs.writeFileSync(p.entry, source(speed));
    await new Promise((r) => setTimeout(r, 2500));  // export time + any echo from writing dist/
    assert.equal(exports, 1);
    assert.match(fs.readFileSync(path.join(p.out, "main.js"), "utf8"), /= 3/);
  } finally {
    w.close();
    fs.rmSync(p.root, { recursive: true, force: true });
  }
});
