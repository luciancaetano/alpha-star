import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { exportPack } from "../lib/pack.mjs";
import { decodePng, encodePng } from "../lib/png.mjs";

// A throwaway game project: src/main.ts plus the given files.
function project(assets, files) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-proj-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: "a" | "b"; images: "i"; tags: never; scenes: { play: undefined }; ui: { hud: "score" } }
const { scene, game } = define<T>();
const play = scene("play", {});
export default game({
  config: { title: "t", size: [10, 10], seed: 1, actions: {} },
  start: "play",
  scenes: { play },
  assets: ${assets},
});
`);
  for (const [file, text] of Object.entries(files)) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), text);
  }
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "dist") };
}

const ASSETS = `{ sounds: { a: "assets/sfx/a.wav", b: { wave: "sine", freq: 440, duration: 0.1 } },
                  images: { i: "assets/img/i.png" } }`;

test("export writes main.js and copies exactly the declared files", async () => {
  const p = project(ASSETS, { "assets/sfx/a.wav": "WAV", "assets/img/i.png": "PNG", "assets/unused.png": "x" });
  const { ok, problems, files } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  assert.equal(files.length, 2);
  assert.match(fs.readFileSync(path.join(p.out, "main.js"), "utf8"), /from "alpha"/);
  assert.equal(fs.readFileSync(path.join(p.out, "assets/sfx/a.wav"), "utf8"), "WAV");
  assert.equal(fs.readFileSync(path.join(p.out, "assets/img/i.png"), "utf8"), "PNG");
  assert.equal(fs.existsSync(path.join(p.out, "assets/unused.png")), false);
});

test("export refuses missing files and paths outside the project, writing nothing", async () => {
  const missing = project(ASSETS, { "assets/sfx/a.wav": "WAV" });
  const r1 = await exportPack(missing);
  assert.equal(r1.ok, false);
  assert.match(r1.problems[0].message, /assets\.images\.i: file "assets\/img\/i\.png" not found/);
  assert.equal(fs.existsSync(missing.out), false);

  const escape = project(`{ sounds: { a: "../a.wav", b: "/abs.wav" }, images: { i: "i.png" } }`, { "i.png": "" });
  const r2 = await exportPack(escape);
  assert.equal(r2.problems.length, 2);
  assert.match(r2.problems[0].message, /must be a relative path inside the project/);
});

test("export stops on type errors before touching assets", async () => {
  const p = project(`{ sounds: { a: "x.wav", c: "y.wav" } }`, {});
  const { ok, problems } = await exportPack(p);
  assert.equal(ok, false);
  assert.match(problems[0].message, /'c'|"c"|images/);
  assert.equal(fs.existsSync(p.out), false);
});

test("the pack's source map names sources relative to the pack, so errors show src/...", async () => {
  const p = project(ASSETS, { "assets/sfx/a.wav": "WAV", "assets/img/i.png": "PNG" });
  await exportPack(p);
  const js = fs.readFileSync(path.join(p.out, "main.js"), "utf8");
  const b64 = js.match(/sourceMappingURL=data:application\/json;base64,(\S+)/)[1];
  const map = JSON.parse(Buffer.from(b64, "base64").toString("utf8"));
  assert.deepEqual(map.sources, ["../src/main.ts"]);
});

const UI_ASSETS = `{ sounds: { a: "assets/sfx/a.wav", b: "assets/sfx/a.wav" }, images: { i: "assets/img/i.png" },
                     fonts: { pixel: "assets/fonts/p.ttf" }, ui: { hud: "assets/ui/hud.rml" } }`;
const UI_FILES = {
  "assets/sfx/a.wav": "WAV",
  "assets/img/i.png": "PNG",
  "assets/fonts/p.ttf": "TTF",
  "assets/ui/hud.rml": `<rml><head><link type="text/rcss" href="../style/main.rcss"/></head>
    <body><img src="logo.png"/><a href="#top">x</a><p id="score" style="decorator: image(inline.png);"/></body></rml>`,
  "assets/style/main.rcss": `body { decorator: image( ../img/bg.png ); } p { background: url("dot.png"); }`,
  "assets/ui/logo.png": "LOGO",
  "assets/ui/inline.png": "IN",
  "assets/img/bg.png": "BG",
  "assets/style/dot.png": "DOT",
};

test("export copies fonts, ui documents and every file they reference", async () => {
  const p = project(UI_ASSETS, { ...UI_FILES, "assets/ui/unused.png": "x" });
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  for (const file of Object.keys(UI_FILES)) {
    assert.equal(fs.readFileSync(path.join(p.out, file), "utf8"), UI_FILES[file], file);
  }
  assert.equal(fs.existsSync(path.join(p.out, "assets/ui/unused.png")), false);
});

test("export names the document and the missing file it references", async () => {
  const { "assets/img/bg.png": _gone, ...files } = UI_FILES;
  const p = project(UI_ASSETS, files);
  const { ok, problems } = await exportPack(p);
  assert.equal(ok, false);
  assert.match(problems[0].message, /assets\.ui\.hud: assets\/style\/main\.rcss references "\.\.\/img\/bg\.png", not found/);
  assert.equal(fs.existsSync(p.out), false);
});

// ---- sprite sheets, atlas, maps --------------------------------------------

function solidPng(width, height, rgba) {
  const px = Buffer.alloc(width * height * 4);
  for (let i = 0; i < width * height; i++) px.set(rgba, i * 4);
  return encodePng({ width, height, rgba: px });
}

const SPRITE_ASSETS = `{ sounds: { a: "assets/a.wav", b: "assets/a.wav" }, images: { i: "img/coin.png" },
  sheets: { hero: { image: "img/hero.png", frame: [4, 4], animations: { run: { frames: [0, 1] } } } } }`;
const SPRITE_FILES = () => ({
  "assets/a.wav": "WAV",
  "img/coin.png": solidPng(3, 3, [255, 0, 0, 255]),
  "img/hero.png": solidPng(8, 4, [0, 0, 255, 255]),
});

test("export packs images and sheet images into an atlas page the engine maps by file name", async () => {
  const p = project(SPRITE_ASSETS, SPRITE_FILES());
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  const manifest = JSON.parse(fs.readFileSync(path.join(p.out, "alpha-atlas.json"), "utf8"));
  assert.deepEqual(Object.keys(manifest.images).sort(), ["img/coin.png", "img/hero.png"]);
  const [page, [x, y, w, h]] = manifest.images["img/coin.png"];
  assert.equal(page, "alpha-atlas-0.png");
  assert.deepEqual([w, h], [3, 3]);
  const atlas = decodePng(fs.readFileSync(path.join(p.out, page)));
  assert.deepEqual([...atlas.rgba.subarray(((y + 1) * atlas.width + x + 1) * 4, ((y + 1) * atlas.width + x + 2) * 4)], [255, 0, 0, 255]);
  assert.equal(fs.existsSync(path.join(p.out, "img/coin.png")), false, "packed images are not copied");
});

test("export with atlas: false copies images as they are", async () => {
  const p = project(SPRITE_ASSETS, SPRITE_FILES());
  const { ok } = await exportPack({ ...p, atlas: false });
  assert.ok(ok);
  assert.equal(fs.existsSync(path.join(p.out, "alpha-atlas.json")), false);
  assert.ok(fs.existsSync(path.join(p.out, "img/hero.png")));
});

const MAP_ASSETS = `{ sounds: { a: "assets/a.wav", b: "assets/a.wav" }, maps: { level1: "maps/level1.tmj" } }`;
const MAP_FILES = {
  "assets/a.wav": "WAV",
  "maps/level1.tmj": JSON.stringify({ tilesets: [{ firstgid: 1, source: "../tilesets/ground.tsj" },
                                                 { firstgid: 9, image: "deco.png", tilewidth: 8, tileheight: 8 }] }),
  "tilesets/ground.tsj": JSON.stringify({ image: "../img/ground.png" }),
  "img/ground.png": "PNG",
  "maps/deco.png": "PNG2",
};

test("export copies maps with their tilesets and tileset images", async () => {
  const p = project(MAP_ASSETS, MAP_FILES);
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  for (const file of ["maps/level1.tmj", "tilesets/ground.tsj", "img/ground.png", "maps/deco.png"]) {
    assert.ok(fs.existsSync(path.join(p.out, file)), file);
  }
});

test("export names the map and the tileset it cannot use", async () => {
  const { "img/ground.png": _gone, ...files } = MAP_FILES;
  const r1 = await exportPack(project(MAP_ASSETS, files));
  assert.equal(r1.ok, false);
  assert.match(r1.problems[0].message, /assets\.maps\.level1: tilesets\/ground\.tsj references "\.\.\/img\/ground\.png", not found/);
  const tsx = { ...MAP_FILES, "maps/level1.tmj": JSON.stringify({ tilesets: [{ firstgid: 1, source: "a.tsx" }] }) };
  const r2 = await exportPack(project(MAP_ASSETS, tsx));
  assert.match(r2.problems[0].message, /a\.tsx: save tilesets as JSON \(\.tsj\)/);
});
