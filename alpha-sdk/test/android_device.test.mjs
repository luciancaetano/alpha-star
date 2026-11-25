import { test } from "node:test";
import assert from "node:assert/strict";
import path from "node:path";
import { adbPath, deviceSteps, pickDevice, readyDevices } from "../lib/android_device.mjs";

test("adb comes from the SDK's platform-tools, else PATH", () => {
  assert.equal(adbPath("C:\\sdk", "win32"), path.join("C:\\sdk", "platform-tools", "adb.exe"));
  assert.equal(adbPath(null, "linux"), "adb");
});

test("only devices adb reports ready are used", () => {
  const listing = "List of devices attached\r\nRXGL401PYVF\tdevice\r\nemu-5554\toffline\r\nABC\tunauthorized\r\n\r\n";
  assert.deepEqual(readyDevices(listing), ["RXGL401PYVF"]);
  assert.deepEqual(readyDevices("List of devices attached\n\n"), []);
});

test("one ready device is picked; none, several or a missing serial say what to do", () => {
  assert.deepEqual(pickDevice(["A"]), { serial: "A" });
  assert.deepEqual(pickDevice(["A", "B"], "B"), { serial: "B" });
  assert.match(pickDevice([]).problem, /USB debugging/);
  assert.match(pickDevice(["A", "B"]).problem, /--device <serial>/);
  assert.match(pickDevice(["A"], "Z").problem, /device "Z" is not connected; ready: A/);
});

test("install, start SDL's activity and follow the engine's log on that device", () => {
  const s = deviceSteps("A", "app-debug.apk", "com.studio.game");
  assert.deepEqual(s.install, ["-s", "A", "install", "-r", "app-debug.apk"]);
  assert.deepEqual(s.start, ["-s", "A", "shell", "am", "start", "-n", "com.studio.game/org.libsdl.app.SDLActivity"]);
  assert.deepEqual(s.log.slice(0, 5), ["-s", "A", "logcat", "-s", "SDL"]);
  assert.ok(s.log.includes("SDL/APP"));
});
