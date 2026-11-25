// `alpha run --target android-arm64`: the debug APK on the connected phone, with its log here.
//   adb install -r <apk>, start SDL's activity, then follow logcat (SDL + the game's console.log)
//   until Ctrl+C. --device <serial> picks one when several are connected.
import { spawnSync, spawn } from "node:child_process";
import path from "node:path";

const ACTIVITY = "org.libsdl.app.SDLActivity";

// The adb inside the Android SDK (platform-tools), else the one on PATH.
export function adbPath(sdk, platform = process.platform) {
  const exe = platform === "win32" ? "adb.exe" : "adb";
  return sdk ? path.join(sdk, "platform-tools", exe) : exe;
}

// Serials of the devices adb lists as ready ("device"), from `adb devices` output.
export function readyDevices(listing) {
  return listing.split(/\r?\n/).slice(1).map((l) => l.trim().split(/\s+/))
    .filter(([serial, state]) => serial && state === "device").map(([serial]) => serial);
}

// The one device to use, or a problem naming what to do.
export function pickDevice(ready, wanted) {
  if (wanted) return ready.includes(wanted) ? { serial: wanted } : { problem: `device "${wanted}" is not connected; ready: ${ready.join(", ") || "none"}` };
  if (ready.length === 1) return { serial: ready[0] };
  if (!ready.length) return { problem: "no Android device ready: plug the phone in by USB, turn on USB debugging (Developer options) and accept the prompt on the phone; `adb devices` lists it as \"device\"" };
  return { problem: `several devices are connected (${ready.join(", ")}): pass --device <serial>` };
}

// The adb argument lists, in order, that install and start the game, then follow its log.
export function deviceSteps(serial, apk, pkg) {
  const on = ["-s", serial];
  return {
    install: [...on, "install", "-r", apk],
    clearLog: [...on, "logcat", "-c"],
    start: [...on, "shell", "am", "start", "-n", `${pkg}/${ACTIVITY}`],
    log: [...on, "logcat", "-s", "SDL", "SDL/APP", "AndroidRuntime", "DEBUG"],
  };
}

const adbRun = (adb, args) => spawnSync(adb, args, { encoding: "utf8" });

// Installs and starts the APK on the device, then streams its log; resolves with an exit code.
export function runOnDevice({ sdk, apk, pkg, device }) {
  const adb = adbPath(sdk);
  const listing = adbRun(adb, ["devices"]);
  if (listing.error) return Promise.resolve(fail(`cannot run adb (${adb}): ${listing.error.message}; install "platform-tools" with sdkmanager`));
  const pick = pickDevice(readyDevices(listing.stdout), device);
  if (pick.problem) return Promise.resolve(fail(pick.problem));
  const steps = deviceSteps(pick.serial, apk, pkg);
  for (const step of ["install", "clearLog", "start"]) {
    const r = adbRun(adb, steps[step]);
    if (r.status !== 0) return Promise.resolve(fail(`adb ${steps[step].join(" ")} failed: ${(r.stderr || r.stdout).trim()}`));
  }
  console.log(`running ${pkg} on ${pick.serial}; its log follows (Ctrl+C stops following, the game keeps running)`);
  const log = spawn(adb, steps.log, { stdio: "inherit" });
  return new Promise((resolve) => log.once("exit", () => resolve(0)));
}

function fail(message) {
  console.error(message);
  return 1;
}
