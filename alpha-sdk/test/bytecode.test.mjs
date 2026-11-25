import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { stampOf } from "../lib/bytecode.mjs";
import { bundledEngine, engineFile, hostTarget, shippingEngine } from "../lib/engine.mjs";
import { exportPack } from "../lib/pack.mjs";

const ENGINE_DIR = path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))), "engine");
const FAKE = "testos-x98";
const staged = Boolean(bundledEngine() && shippingEngine());
const SECRET = "SECRET_SOURCE_COMMENT_abc123";

function project() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-jsbc-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: never; images: never; tags: never; scenes: { play: undefined }; ui: {} }
const { scene, game } = define<T>();
// ${SECRET}
function secretHelper(n: number): number { return n * 7 + 3; }
export default game({ config: { title: "jsbc", size: [32, 32], seed: 1, actions: {} }, start: "play",
  scenes: { play: scene("play", { update() { secretHelper(1); } }) }, assets: { sounds: {} } });
`);
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "dist") };
}

function header(engine, quickjs, build) {
  const b = Buffer.alloc(64);
  b.write("ALPHAJBC", 0, "latin1");
  b.write(engine, 24, "latin1");
  b.write(quickjs, 36, "latin1");
  b.write(build, 48, "latin1");
  return b;
}

test("stampOf reads a game.jsbc header and rejects anything else", () => {
  assert.deepEqual(stampOf(header("0.2.0", "0.10.1", "abc-64")), { engine: "0.2.0", quickjs: "0.10.1", build: "abc-64" });
  assert.equal(stampOf(Buffer.from("export default 1;")), null);
  assert.equal(stampOf(Buffer.alloc(64)), null);
});

test("standalone export without a compiler says how to fix it and writes nothing", async () => {
  const dir = path.join(ENGINE_DIR, FAKE, "shipping");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, engineFile(FAKE)), "SHIPPING");
  try {
    const p = project();
    const { ok, problems } = await exportPack({ ...p, target: FAKE, compiler: null });
    assert.equal(ok, false);
    assert.match(problems[0].message, /no release engine .*--no-bytecode/);
    assert.equal(fs.existsSync(p.out), false);
  } finally {
    fs.rmSync(path.join(ENGINE_DIR, FAKE), { recursive: true, force: true });
  }
});

test("a shipping engine from another build is refused before anything is written", { skip: !staged }, async () => {
  const dir = path.join(ENGINE_DIR, FAKE, "shipping");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, engineFile(FAKE)), "SHIPPING");
  try {
    const p = project();
    const { ok, problems } = await exportPack({ ...p, target: FAKE });
    assert.equal(ok, false);
    assert.match(problems[0].message, /different builds; restage both/);
    assert.equal(fs.existsSync(p.out), false);
  } finally {
    fs.rmSync(path.join(ENGINE_DIR, FAKE), { recursive: true, force: true });
  }
});

test("standalone export ships game.jsbc, no JS source, next to the unmodified engine", { skip: !staged }, async () => {
  const p = project();
  const { ok, problems, game } = await exportPack({ ...p, target: hostTarget() });
  assert.deepEqual(problems, []);
  assert.ok(ok);
  assert.equal(game, "game.jsbc");
  const names = fs.readdirSync(p.out);
  assert.ok(names.includes("game.jsbc"));
  assert.ok(!names.some((n) => n.endsWith(".js") || n.endsWith(".map")), names.join(", "));
  const jsbc = fs.readFileSync(path.join(p.out, "game.jsbc"));
  assert.ok(!jsbc.includes(SECRET) && !jsbc.includes("n * 7 + 3") && !jsbc.includes("sourceMappingURL"));
  assert.ok(fs.readFileSync(path.join(p.out, engineFile(hostTarget()))).equals(fs.readFileSync(shippingEngine())));
  // The release engine (same build) runs the pack headlessly: proof the bytecode boots.
  const run = spawnSync(bundledEngine(), [p.out, "--frames", "3", "--hidden"], { encoding: "utf8" });
  assert.equal(run.status, 0, run.stderr);
  assert.doesNotMatch(run.stderr, /error|game\.jsbc/i);
});
