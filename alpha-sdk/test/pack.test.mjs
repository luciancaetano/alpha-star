import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { exportPack } from "../lib/pack.mjs";
import { decodePng, encodePng } from "../lib/png.mjs";

// A throwaway game project: src/main.ts plus the given files.
function project(assets, files, images = `"i"`) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-proj-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: "a" | "b"; images: ${images}; tags: never; scenes: { play: undefined }; ui: { hud: "score" } }
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
  "assets/style/main.rcss": `body { decorator: image( ../img/bg.png ); } p { background: url("dot.png"); }
    .card { decorator: image(card.png fill); } .b { decorator: image( "b.png" contain center bottom ); }
    @spritesheet icons { src: sheet.png; heart: 0px 0px 16px 16px; }`,
  "assets/ui/logo.png": "LOGO",
  "assets/ui/inline.png": "IN",
  "assets/img/bg.png": "BG",
  "assets/style/dot.png": "DOT",
  "assets/style/card.png": "CARD",
  "assets/style/b.png": "B",
  "assets/style/sheet.png": "SHEET",
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
  assert.deepEqual(Object.keys(manifest.filters.nearest).sort(), ["img/coin.png", "img/hero.png"]);
  const [page, [x, y, w, h]] = manifest.filters.nearest["img/coin.png"];
  assert.equal(page, "alpha-atlas-0.png");
  assert.deepEqual([w, h], [3, 3]);
  const atlas = decodePng(fs.readFileSync(path.join(p.out, page)));
  assert.deepEqual([...atlas.rgba.subarray(((y + 1) * atlas.width + x + 1) * 4, ((y + 1) * atlas.width + x + 2) * 4)], [255, 0, 0, 255]);
  assert.equal(fs.existsSync(path.join(p.out, "img/coin.png")), false, "packed images are not copied");
});

test("export puts each image filter (nearest, linear, sharp) on its own atlas pages", async () => {
  const assets = `{ sounds: { a: "assets/a.wav", b: "assets/a.wav" },
    images: { coin: "img/coin.png", glow: { file: "img/glow.png", filter: "linear" }, coinSoft: { file: "img/coin.png", filter: "linear" },
              coinSharp: { file: "img/coin.png", filter: "sharp" } },
    sheets: { hero: { image: "img/hero.png", frame: [4, 4], filter: "linear" } } }`;
  const p = project(assets, { ...SPRITE_FILES(), "img/glow.png": solidPng(2, 2, [0, 255, 0, 255]) }, `"coin" | "glow" | "coinSoft" | "coinSharp"`);
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  const manifest = JSON.parse(fs.readFileSync(path.join(p.out, "alpha-atlas.json"), "utf8"));
  assert.deepEqual(Object.keys(manifest.filters.nearest), ["img/coin.png"]);
  assert.deepEqual(Object.keys(manifest.filters.linear).sort(), ["img/coin.png", "img/glow.png", "img/hero.png"]);
  assert.deepEqual(Object.keys(manifest.filters.sharp), ["img/coin.png"]);
  assert.equal(manifest.filters.nearest["img/coin.png"][0], "alpha-atlas-0.png");
  assert.equal(manifest.filters.linear["img/glow.png"][0], "alpha-atlas-linear-0.png");
  assert.equal(manifest.filters.sharp["img/coin.png"][0], "alpha-atlas-sharp-0.png");
  assert.deepEqual(manifest.pages, { "alpha-atlas-0.png": { filter: "nearest" }, "alpha-atlas-linear-0.png": { filter: "linear" },
                                     "alpha-atlas-sharp-0.png": { filter: "sharp" } });
});

test("export with atlas: false copies images as they are", async () => {
  const p = project(SPRITE_ASSETS, SPRITE_FILES());
  const { ok } = await exportPack({ ...p, atlas: false });
  assert.ok(ok);
  assert.equal(fs.existsSync(path.join(p.out, "alpha-atlas.json")), false);
  assert.ok(fs.existsSync(path.join(p.out, "img/hero.png")));
});

const WORLD_ASSETS = `{ sounds: { a: "assets/a.wav", b: "assets/a.wav" }, worlds: { main: "levels/main.ldtk" } }`;
const WORLD_FILES = {
  "assets/a.wav": "WAV",
  "levels/main.ldtk": JSON.stringify({
    defs: { tilesets: [{ relPath: "../img/ground.png" }, { relPath: null, embedAtlas: "LdtkIcons" }] },
    levels: [{ identifier: "A" }, { identifier: "B", externalRelPath: "main/B.ldtkl" }],
  }),
  "img/ground.png": "PNG",
  "levels/main/B.ldtkl": "{}",
};

test("export copies LDtk worlds with their tileset images and external levels", async () => {
  const p = project(WORLD_ASSETS, WORLD_FILES);
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  for (const file of ["levels/main.ldtk", "img/ground.png", "levels/main/B.ldtkl"]) {
    assert.ok(fs.existsSync(path.join(p.out, file)), file);
  }
});

test("export names the world and the file it cannot find", async () => {
  const { "img/ground.png": _gone, ...files } = WORLD_FILES;
  const r = await exportPack(project(WORLD_ASSETS, files));
  assert.equal(r.ok, false);
  assert.match(r.problems[0].message, /assets\.worlds\.main: levels\/main\.ldtk references "\.\.\/img\/ground\.png", not found/);
});

test("export copies sound files in { file, bus }, music and locale files", async () => {
  const p = project(`{ sounds: { a: { file: "assets/a.ogg", bus: "voice" }, b: { synth: { wave: "sine", freq: 1, duration: 1 } } },
                       images: { i: "assets/i.png" }, music: { theme: "assets/music/theme.mp3" },
                       locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" } }`,
    { "assets/a.ogg": "OGG", "assets/i.png": "PNG", "assets/music/theme.mp3": "MP3",
      "assets/locales/en.json": "{}", "assets/locales/pt.json": "{}" });
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  for (const f of ["assets/a.ogg", "assets/music/theme.mp3", "assets/locales/en.json", "assets/locales/pt.json"]) {
    assert.ok(fs.existsSync(path.join(p.out, f)), f);
  }
});

test("export writes THIRD_PARTY_NOTICES.txt: engine deps (FreeType credit) and each font's license", async () => {
  const fnt = 'info size=8\ncommon lineHeight=8 base=7 pages=1\npage id=0 file="tiny_0.png"\nchar id=65 x=0 y=0 width=1 height=1 xadvance=1 page=0\n';
  const p = project(`{ sounds: { a: "a.wav", b: "a.wav" }, images: { i: "i.png" },
                       fonts: { ui: { file: "fonts/p.ttf", license: "fonts/OFL.txt" }, tiny: { file: "fonts/tiny.fnt", pixelMode: true } } }`,
                    { "a.wav": "WAV", "i.png": "PNG", "fonts/p.ttf": "TTF", "fonts/OFL.txt": "SIL Open Font License 1.1 text",
                      "fonts/tiny.fnt": fnt, "fonts/tiny_0.png": "PNG0" });
  const { ok, problems } = await exportPack({ ...p, atlas: false });
  assert.deepEqual(problems, []);
  assert.ok(ok);
  const notices = fs.readFileSync(path.join(p.out, "THIRD_PARTY_NOTICES.txt"), "utf8");
  assert.match(notices, /Portions of this software are copyright \(c\) \d{4} The FreeType Project \(www\.freetype\.org\)/);
  for (const dep of ["SDL 3", "SDL_image", "Box2D", "EnTT", "QuickJS-ng", "RmlUi", "stb_vorbis", "dr_mp3", "Built-in 8x8 font"]) {
    assert.ok(notices.includes(dep), dep);
  }
  assert.ok(!notices.includes("Dear ImGui"));  // dev tools never ship
  assert.match(notices, /Font "ui" \(fonts\/p\.ttf\)[\s\S]*SIL Open Font License 1\.1 text/);
  assert.equal(fs.readFileSync(path.join(p.out, "fonts/OFL.txt"), "utf8"), "SIL Open Font License 1.1 text");
  assert.equal(fs.readFileSync(path.join(p.out, "fonts/tiny_0.png"), "utf8"), "PNG0");  // BMFont page

  const missing = project(`{ sounds: { a: "a.wav", b: "a.wav" }, images: { i: "i.png" }, fonts: { t: "t.fnt" } }`,
                          { "a.wav": "W", "i.png": "P", "t.fnt": 'common lineHeight=8\npage id=0 file="gone.png"\n' });
  const r = await exportPack(missing);
  assert.match(r.problems[0].message, /assets\.fonts\.t: t\.fnt references "gone\.png", not found/);
});

test("export evaluates games that use every value export of \"alpha\" (vec at module level too)", async () => {
  const p = project(`{ sounds: { a: { wave: "sine", freq: 440, duration: 0.1 }, b: { wave: "sine", freq: 220, duration: 0.1 } } }`, {}, "never");
  const main = fs.readFileSync(p.entry, "utf8");
  fs.writeFileSync(p.entry, main.replace(`import { define } from "alpha";`,
    `import { define, vec } from "alpha";\nexport const START = vec.add(vec.scale(vec.fromAngle(0, 2), 2), vec.norm([3, 4]));`));
  const { ok, problems } = await exportPack(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
});

test("the exporter's alpha shim has every export of the engine's alpha module", async (t) => {
  const engine = new URL("../../src/script/alpha.js", import.meta.url);
  if (!fs.existsSync(engine)) return t.skip("engine sources not next to this SDK");
  const names = [...fs.readFileSync(engine, "utf8").matchAll(/^export (?:function|const) (\w+)/gm)].map((m) => m[1]);
  const shim = await import("../lib/alpha-shim.mjs");
  for (const name of names) assert.ok(name in shim, `alpha-shim.mjs lacks "${name}"`);
});
