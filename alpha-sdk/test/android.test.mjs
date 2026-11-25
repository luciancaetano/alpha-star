import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { androidValues, defaultPackage, exportAndroid, findToolchain, orientationFor, SDK_PACKAGES, TEMPLATE, VARIANTS }
  from "../lib/android.mjs";
import { stampOf } from "../lib/bytecode.mjs";
import { bundledEngine } from "../lib/engine.mjs";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");

function project(config) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-android-proj-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.mkdirSync(path.join(root, "assets"));
  fs.writeFileSync(path.join(root, "assets", "a.wav"), "WAV");
  fs.writeFileSync(path.join(root, "icon.png"), "PNGDATA");
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: "back"; sounds: "a"; images: never; tags: never; scenes: { play: undefined }; ui: never }
const { scene, game } = define<T>();
const play = scene("play", {});
export default game({
  config: ${config},
  start: "play",
  scenes: { play },
  assets: { sounds: { a: "assets/a.wav" } },
});
`);
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "build", "android") };
}

const read = (...parts) => fs.readFileSync(path.join(...parts), "utf8");

test("android export assembles a Gradle project: pack in assets, ids and orientation from the config", async () => {
  const p = project(`{ title: "Flappy <Star>", size: [360, 640], seed: 1, actions: { back: ["Escape", "Back"] },
                       android: { package: "com.studio.flappy", icon: "icon.png", versionCode: 7, version: "1.2" } }`);
  const r = await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false });
  assert.deepEqual(r.problems, []);
  assert.ok(r.ok);
  const main = path.join(p.out, "app", "src", "main");
  // The APK ships bytecode compiled by this machine's release engine (no JS source).
  const game = bundledEngine() ? "game.jsbc" : "main.js";
  assert.deepEqual(fs.readdirSync(path.join(main, "assets")).filter((f) => /\.(js|jsbc|map)$/.test(f)), [game]);
  if (bundledEngine()) assert.match(stampOf(fs.readFileSync(path.join(main, "assets", game))).build, /-64$/);
  assert.equal(read(main, "assets", "assets", "a.wav"), "WAV");
  const gradle = read(p.out, "app", "build.gradle");
  assert.match(gradle, /applicationId "com\.studio\.flappy"/);
  assert.match(gradle, /versionCode 7/);
  assert.match(gradle, /versionName "1\.2"/);
  // The engine path is relative to app/, so a committed project builds in any clone.
  const engine = path.relative(path.join(p.out, "app"), path.join(REPO, "CMakeLists.txt")).split(path.sep).join("/");
  assert.ok(gradle.includes(`path file("${engine}")`), gradle.match(/path file\(.*\)/)?.[0]);
  // debug APKs carry the dev tools (release engine flavor); release APKs and AABs ship without them.
  assert.match(gradle, /debug \{\s*externalNativeBuild \{ cmake \{ arguments "-DALPHA_SHIPPING=OFF" \} \}/);
  assert.match(gradle, /release \{[^}]*signingConfig.*\s*externalNativeBuild \{ cmake \{ arguments "-DALPHA_SHIPPING=ON" \} \}/);
  assert.match(gradle, /abiFilters 'arm64-v8a'/);
  assert.match(gradle, /-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON/);  // Google Play: 16 KB pages
  assert.match(gradle, /System\.getenv\("ALPHA_ANDROID_KEYSTORE"\)/);  // release key from env, none stored
  assert.doesNotMatch(gradle, /\{\{/);
  assert.match(read(main, "AndroidManifest.xml"), /android:screenOrientation="sensorPortrait"/);
  assert.match(read(main, "res", "values", "strings.xml"), /Flappy &lt;Star&gt;/);
  assert.equal(read(main, "res", "mipmap-hdpi", "ic_launcher.png"), "PNGDATA");
  assert.ok(fs.existsSync(path.join(main, "java", "org", "libsdl", "app", "SDLActivity.java")));
  assert.ok(fs.existsSync(path.join(p.out, "gradlew.bat")));
  // No toolchain in this environment: nothing built, the help names every package and the command.
  assert.equal(r.apk, null);
  for (const pkg of SDK_PACKAGES) assert.ok(r.help.includes(pkg), pkg);
  assert.match(r.help, /assembleRelease/);
  assert.match(r.help, /JDK 17/);
});

test("android export without bytecode ships main.js; without a compiler it says how to fix it", async () => {
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] } }`);
  const plain = await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false, bytecode: false });
  assert.ok(plain.ok);
  assert.ok(fs.existsSync(path.join(p.out, "app", "src", "main", "assets", "main.js")));
  const none = await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false, compiler: null });
  assert.match(none.problems[0].message, /no release engine .*--no-bytecode/);
});

test("exporting again keeps Gradle's caches and replaces the pack whole", async () => {
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] } }`);
  const opts = { ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false, bytecode: false };
  assert.ok((await exportAndroid(opts)).ok);
  const cache = path.join(p.out, "app", "build", "marker");
  const stale = path.join(p.out, "app", "src", "main", "assets", "removed.png");
  fs.mkdirSync(path.dirname(cache), { recursive: true });
  fs.writeFileSync(cache, "");
  fs.writeFileSync(stale, "");
  assert.ok((await exportAndroid(opts)).ok);
  assert.ok(fs.existsSync(cache));
  assert.equal(fs.existsSync(stale), false);
});

test("variants: debug, release and bundle map to their Gradle tasks; others are rejected", async () => {
  assert.deepEqual(Object.fromEntries(Object.entries(VARIANTS).map(([k, v]) => [k, v.task])),
                   { debug: "assembleDebug", release: "assembleRelease", bundle: "bundleRelease" });
  assert.equal(VARIANTS.bundle.output.at(-1), "app-release.aab");
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] } }`);
  const r = await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false, variant: "beta" });
  assert.match(r.problems[0].message, /unknown --variant "beta"; variants: debug, release, bundle/);
});

test("android export rejects a bad package id, an unknown target and a missing engine source", async () => {
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] }, android: { package: "nodots" } }`);
  const bad = await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false });
  assert.match(bad.problems[0].message, /config\.android\.package "nodots" is not a package id/);
  const target = await exportAndroid({ ...p, target: "android-x86", engineSrc: REPO, env: {}, build: false });
  assert.match(target.problems[0].message, /unknown target "android-x86"; Android targets: android-arm64/);
  assert.equal(fs.existsSync(p.out), false);
});

test("defaults: package from the title, landscape unless taller than wide", () => {
  assert.equal(defaultPackage("Flappy Star!"), "org.alphastar.flappystar");
  assert.equal(defaultPackage("2048"), "org.alphastar.game2048");
  assert.equal(orientationFor([800, 480]), "sensorLandscape");
  assert.equal(orientationFor([480, 480]), "sensorLandscape");
  const { values, problems } = androidValues({ title: "It's", size: [1, 2] }, REPO);
  assert.deepEqual(problems, []);
  assert.equal(values.APP_NAME, String.raw`It\'s`);  // Android string resources escape apostrophes
  assert.equal(values.VERSION_CODE, "1");
  assert.match(androidValues({ android: { versionCode: 0 } }, REPO).problems[0].message, /versionCode/);
});

test("toolchain lookup reports each missing sdkmanager package and the JDK", () => {
  const sdk = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-fake-sdk-"));
  fs.mkdirSync(path.join(sdk, "platforms", "android-35"), { recursive: true });
  const t = findToolchain({ ANDROID_HOME: sdk });
  assert.equal(t.sdk, sdk);
  assert.deepEqual(t.missing, SDK_PACKAGES.slice(1).concat("JDK 17+ (JAVA_HOME)"));
});

test("the Java glue is SDL's own, version-matched with the engine's SDL", { skip: !fs.existsSync(path.join(REPO, "build", "dev", "_deps", "sdl3-src")) }, () => {
  const sdl = path.join(REPO, "build", "dev", "_deps", "sdl3-src", "android-project", "app", "src", "main", "java", "org", "libsdl", "app");
  const ours = path.join(TEMPLATE, "app", "src", "main", "java", "org", "libsdl", "app");
  assert.deepEqual(fs.readdirSync(ours).sort(), fs.readdirSync(sdl).sort());
  const lf = (s) => s.replace(/\r\n/g, "\n");  // git may check either copy out with other line endings
  for (const f of fs.readdirSync(sdl)) assert.equal(lf(read(ours, f)), lf(read(sdl, f)), f);
  assert.match(fs.readFileSync(path.join(REPO, "cmake", "Dependencies.cmake"), "utf8"), /release-3\.2\.10/);
  assert.match(read(ours, "SDLActivity.java"), /SDL_MICRO_VERSION = 10;/);
});
