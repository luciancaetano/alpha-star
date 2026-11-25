// `alpha export --target android-arm64`: a ready-to-build Gradle project.
//   <out>/                  the SDK's android/ template (SDL 3.2.10 Java glue, Gradle wrapper)
//   <out>/app/src/main/assets  the game pack (game.jsbc + assets/...), read through SDL's APK asset IO
// The engine's libmain.so is built from the alpha-star source with the shipping flavor
// (CMake externalNativeBuild). When the Android toolchain is installed, Gradle builds the
// APK at once; otherwise the exporter says exactly what to install and how to build.
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { bundledEngine } from "./engine.mjs";
import { exportPack, readGameDef } from "./pack.mjs";

const SDK = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
export const TEMPLATE = path.join(SDK, "android");
export const ANDROID_TARGETS = ["android-arm64"];
const ABI_BITS = { "android-arm64": 64 };  // pointer width in the engine's bytecode build id

// What the Gradle project pins (app/build.gradle); sdkmanager package names.
export const TOOLCHAIN = {
  ndk: "27.2.12479018",
  cmake: "3.31.6",
  platform: "android-35",
  buildTools: "36.0.0",
};
export const SDK_PACKAGES = [`platforms;${TOOLCHAIN.platform}`, `build-tools;${TOOLCHAIN.buildTools}`,
                             `ndk;${TOOLCHAIN.ndk}`, `cmake;${TOOLCHAIN.cmake}`];

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });
const PACKAGE_ID = /^[a-zA-Z][a-zA-Z0-9_]*(\.[a-zA-Z][a-zA-Z0-9_]*)+$/;
const TEXT = new Set([".gradle", ".xml", ".properties", ".pro"]);

// "Flappy Star!" -> "org.alphastar.flappystar": the default package id.
export function defaultPackage(title) {
  const slug = String(title ?? "").toLowerCase().replace(/[^a-z0-9]/g, "");
  return `org.alphastar.${/^[a-z]/.test(slug) ? slug : `game${slug}`}`;
}

// Same rule as the engine (platform::orientations): portrait only when taller than wide.
export function orientationFor(size) {
  return Array.isArray(size) && size[1] > size[0] ? "sensorPortrait" : "sensorLandscape";
}

const xml = (s) => String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
  .replace(/"/g, "\\&quot;").replace(/'/g, "\\'");

// The project's values from config.title / config.size / config.android; problems name the field.
export function androidValues(config, engineSrc) {
  const a = config.android ?? {};
  const values = {
    PACKAGE: a.package ?? defaultPackage(config.title),
    APP_NAME: xml(config.title ?? "Game"),
    PROJECT_NAME: String(config.title ?? "game").replace(/[^A-Za-z0-9_-]/g, "") || "game",
    ORIENTATION: orientationFor(config.size),
    VERSION_CODE: String(a.versionCode ?? 1),
    VERSION_NAME: String(a.version ?? "1.0").replace(/"/g, ""),
    ENGINE_CMAKE: path.join(engineSrc, "CMakeLists.txt").split(path.sep).join("/"),
  };
  const problems = [];
  if (!PACKAGE_ID.test(values.PACKAGE)) {
    problems.push(problem(`config.android.package "${values.PACKAGE}" is not a package id like "com.studio.game" ` +
                          "(dot-separated parts starting with a letter; letters, digits, _)"));
  }
  if (!Number.isInteger(Number(values.VERSION_CODE)) || Number(values.VERSION_CODE) < 1) {
    problems.push(problem(`config.android.versionCode must be a whole number >= 1, got ${a.versionCode}`));
  }
  return { values, problems };
}

// The alpha-star checkout the engine builds from: --engine-src, $ALPHA_ENGINE_SRC, or the repo
// this SDK sits in. null when none has src/main.cpp.
export function findEngineSource(explicit, env = process.env) {
  for (const dir of [explicit, env.ALPHA_ENGINE_SRC, path.dirname(SDK)]) {
    if (dir && fs.existsSync(path.join(dir, "src", "main.cpp"))) return path.resolve(dir);
  }
  return null;
}

function copyTemplate(dir, out, values) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const from = path.join(dir, entry.name);
    const to = path.join(out, entry.name);
    if (entry.isDirectory()) {
      fs.mkdirSync(to, { recursive: true });
      copyTemplate(from, to, values);
    } else if (TEXT.has(path.extname(entry.name))) {
      const text = fs.readFileSync(from, "utf8").replace(/\{\{([A-Z_]+)\}\}/g, (m, k) => values[k] ?? m);
      fs.writeFileSync(to, text);
    } else {
      fs.copyFileSync(from, to);
    }
  }
}

// config.android.icon (a PNG in the project) replaces SDL's launcher icon at every density.
function copyIcon(root, config, out) {
  const icon = config.android?.icon;
  if (!icon) return null;
  const file = path.resolve(root, icon);
  if (!fs.existsSync(file) || path.extname(file).toLowerCase() !== ".png") {
    return problem(`config.android.icon "${icon}" must be a PNG file in the project`, file);
  }
  const res = path.join(out, "app", "src", "main", "res");
  for (const d of fs.readdirSync(res).filter((d) => d.startsWith("mipmap-"))) {
    fs.copyFileSync(file, path.join(res, d, "ic_launcher.png"));
  }
  return null;
}

// The Android SDK the build would use: $ANDROID_HOME, $ANDROID_SDK_ROOT, then the default
// install folder. `missing` lists sdkmanager packages (and a JDK) not found.
export function findToolchain(env = process.env) {
  const home = env.HOME ?? env.USERPROFILE ?? "";
  const candidates = [env.ANDROID_HOME, env.ANDROID_SDK_ROOT,
    env.LOCALAPPDATA && path.join(env.LOCALAPPDATA, "Android", "Sdk"),
    home && path.join(home, "Library", "Android", "sdk"), home && path.join(home, "Android", "Sdk")];
  const sdk = candidates.find((d) => d && fs.existsSync(d)) ?? null;
  const need = [["platforms", TOOLCHAIN.platform], ["build-tools", TOOLCHAIN.buildTools],
                ["ndk", TOOLCHAIN.ndk], ["cmake", TOOLCHAIN.cmake]];
  const missing = need.filter(([kind, ver]) => !sdk || !fs.existsSync(path.join(sdk, kind, ver)))
    .map(([kind, ver]) => `${kind};${ver}`);
  const java = env.JAVA_HOME && fs.existsSync(path.join(env.JAVA_HOME, "bin")) ? env.JAVA_HOME : null;
  if (!java) missing.push("JDK 17+ (JAVA_HOME)");
  return { sdk, java, missing };
}

export function installHelp(out, toolchain) {
  const gradlew = process.platform === "win32" ? "gradlew.bat" : "./gradlew";
  return [
    `Android toolchain missing: ${toolchain.missing.join(", ")}.`,
    "Install the Android SDK command-line tools (https://developer.android.com/studio#command-tools),",
    `unpacked to <sdk>/cmdline-tools/latest${toolchain.sdk ? ` (your SDK: ${toolchain.sdk})` : ""}, then:`,
    `  sdkmanager ${SDK_PACKAGES.map((p) => `"${p}"`).join(" ")}`,
    "and a JDK 17 or newer (JAVA_HOME). Gradle 8.12 comes with the project's wrapper.",
    "Build the APK (debug-signed, arm64-v8a):",
    `  cd ${out} && ${gradlew} assembleRelease`,
    "  -> app/build/outputs/apk/release/app-release.apk",
  ].join("\n");
}

// Gradle under the engine repo's resource guard (one build, 8 CMake jobs).
function runGradle(out, toolchain, engineSrc) {
  const gradlew = path.join(out, process.platform === "win32" ? "gradlew.bat" : "gradlew");
  const guard = path.join(engineSrc, "tools", "resource_guard.py");
  const command = fs.existsSync(guard) ? ["python", [guard, "--", gradlew, "assembleRelease"]]
                                       : [gradlew, ["assembleRelease"]];
  const env = { ...process.env, JAVA_HOME: toolchain.java, ANDROID_HOME: toolchain.sdk, CMAKE_BUILD_PARALLEL_LEVEL: "8" };
  const run = spawnSync(command[0], command[1], { cwd: out, env, stdio: "inherit", shell: process.platform === "win32" });
  const apk = path.join(out, "app", "build", "outputs", "apk", "release", "app-release.apk");
  return run.status === 0 && fs.existsSync(apk) ? apk : null;
}

// Assembles the project in `out`; builds the APK when `build` and the toolchain is there.
// bytecode: the APK ships game.jsbc compiled by this machine's release engine (same
// QuickJS source hash; arm64 is 64-bit like the host), compiler overrides it.
export async function exportAndroid({ root, entry, out, target, engineSrc, atlas = true, env = process.env, build = true,
                                      bytecode = true, compiler = bundledEngine() }) {
  const fail = (problems) => ({ ok: false, problems, files: [] });
  if (!ANDROID_TARGETS.includes(target)) return fail([problem(`unknown target "${target}"; Android targets: ${ANDROID_TARGETS.join(", ")}`)]);
  const source = findEngineSource(engineSrc, env);
  if (!source) {
    return fail([problem("the Android build compiles the engine from source: pass --engine-src <alpha-star checkout> " +
                         "or set ALPHA_ENGINE_SRC (no prebuilt android-arm64 engine is bundled)")]);
  }
  const def = await readGameDef(entry);
  if (!def) return fail([problem("main module must `export default game({...})`", entry)]);
  const { values, problems } = androidValues(def.config ?? {}, source);
  if (problems.length) return fail(problems);
  const staging = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-android-"));
  try {
    const pack = await exportPack({ root, entry, out: staging, atlas, bytecode, compiler, bytecodeBits: ABI_BITS[target] });
    if (!pack.ok) return pack;
    fs.rmSync(out, { recursive: true, force: true });
    fs.mkdirSync(out, { recursive: true });
    copyTemplate(TEMPLATE, out, values);
    fs.cpSync(staging, path.join(out, "app", "src", "main", "assets"), { recursive: true });
    const iconProblem = copyIcon(root, def.config ?? {}, out);
    if (iconProblem) return fail([iconProblem]);
    const toolchain = findToolchain(env);
    if (toolchain.sdk) fs.writeFileSync(path.join(out, "local.properties"), `sdk.dir=${toolchain.sdk.split(path.sep).join("/")}\n`);
    const ready = toolchain.missing.length === 0;
    const apk = build && ready ? runGradle(out, toolchain, source) : null;
    if (build && ready && !apk) return fail([problem(`gradle assembleRelease failed in ${out} (its output is above)`)]);
    return { ok: true, problems: [], files: pack.files, values, toolchain, apk,
             help: ready ? null : installHelp(out, toolchain) };
  } finally {
    fs.rmSync(staging, { recursive: true, force: true });
  }
}
