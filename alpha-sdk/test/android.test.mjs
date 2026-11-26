import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { androidValues, defaultPackage, exportAndroid, findToolchain, orientationFor, SDK_PACKAGES, TEMPLATE, VARIANTS }
  from "../lib/android.mjs";
import { stampOf } from "../lib/bytecode.mjs";
import { bundledEngine, shippingEngine } from "../lib/engine.mjs";

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
  assert.ok(gradle.includes(`def engineCmake = "${engine}"`), gradle.match(/def engineCmake = .*/)?.[0]);
  for (const set of ["debug", "release"]) assert.equal(fs.existsSync(path.join(p.out, "app", "src", set, "jniLibs")), false);
  // debug APKs carry the dev tools (release engine flavor); release APKs and AABs ship without them.
  assert.match(gradle, /debug \{[^}]*externalNativeBuild \{ cmake \{ arguments "-DALPHA_SHIPPING=OFF", "-DALPHA_DEVTOOLS=ON", "-DALPHA_MCP=OFF", "-DALPHA_DEBUG_TOOLS=ON" \} \}/);
  assert.match(gradle, /release \{[^}]*signingConfig.*\s*if \(engineCmake\) externalNativeBuild \{ cmake \{ arguments "-DALPHA_SHIPPING=ON" \} \}/);
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

// A prebuilt engine folder like @alpha-star/engine-android-arm64; libmain.so = `main` bytes.
function prebuiltEngine(main = "ELF main") {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-android-engine-"));
  for (const flavor of ["debug", "shipping"]) {
    fs.mkdirSync(path.join(dir, flavor));
    fs.writeFileSync(path.join(dir, flavor, "libmain.so"), typeof main === "string" ? `${main} ${flavor}` : main);
    fs.writeFileSync(path.join(dir, flavor, "libSDL3.so"), `ELF SDL3 ${flavor}`);
  }
  return dir;
}

test("with a prebuilt engine the project needs no engine source, NDK or CMake", async () => {
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] } }`);
  const prebuilt = prebuiltEngine();
  const r = await exportAndroid({ ...p, target: "android-arm64", env: { ALPHA_ENGINE_SRC: "" }, build: false, bytecode: false, prebuilt });
  assert.deepEqual(r.problems, []);
  const gradle = read(p.out, "app", "build.gradle");
  assert.match(gradle, /def engineCmake = ""/);
  // Each build type packs its own flavor: debug = dev tools, release/bundle = shipping.
  const jni = (set, lib) => read(p.out, "app", "src", set, "jniLibs", "arm64-v8a", lib);
  assert.equal(jni("debug", "libmain.so"), "ELF main debug");
  assert.equal(jni("release", "libmain.so"), "ELF main shipping");
  assert.equal(jni("release", "libSDL3.so"), "ELF SDL3 shipping");
  assert.ok(!r.toolchain.missing.some((m) => /^(ndk|cmake);/.test(m)), r.toolchain.missing.join(", "));
  // An explicit source wins and removes the copied libraries (no duplicate libmain.so in the APK).
  assert.ok((await exportAndroid({ ...p, target: "android-arm64", engineSrc: REPO, env: {}, build: false, bytecode: false, prebuilt })).ok);
  assert.equal(fs.existsSync(path.join(p.out, "app", "src", "release", "jniLibs")), false);
});

test("prebuilt engine: game.jsbc must come from the same engine build", { skip: !bundledEngine() || !shippingEngine() }, async () => {
  const p = project(`{ title: "t", size: [10, 10], seed: 1, actions: { back: [] } }`);
  const other = await exportAndroid({ ...p, target: "android-arm64", env: {}, build: false, prebuilt: prebuiltEngine() });
  assert.match(other.problems[0].message, /different builds: install @alpha-star\/sdk and its engine packages at one version/);
  // The desktop shipping engine carries the same stamp and key strings a matching libmain.so has.
  const same = await exportAndroid({ ...p, target: "android-arm64", env: {}, build: false,
                                     prebuilt: prebuiltEngine(fs.readFileSync(shippingEngine())) });
  assert.deepEqual(same.problems, []);
  assert.ok(fs.existsSync(path.join(p.out, "app", "src", "main", "assets", "game.jsbc")));
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

test("toolchain lookup without a native build needs no NDK or CMake", () => {
  const sdk = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-fake-sdk-"));
  assert.deepEqual(findToolchain({ ANDROID_HOME: sdk }, false).missing,
                   SDK_PACKAGES.slice(0, 2).concat("JDK 17+ (JAVA_HOME)"));
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
  assert.match(fs.readFileSync(path.join(REPO, "cmake", "Dependencies.cmake"), "utf8"), /release-3\.4\.20/);
  assert.match(read(ours, "SDLActivity.java"), /SDL_MICRO_VERSION = 20;/);
});
