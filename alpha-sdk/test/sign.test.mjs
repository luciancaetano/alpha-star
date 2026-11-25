import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { bundledEngine, engineFile, hostTarget, shippingEngine } from "../lib/engine.mjs";
import { signCommand, signFile } from "../lib/sign.mjs";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const CLI = path.join(HERE, "..", "bin", "alpha.mjs");
const REPO = path.join(HERE, "..", "..");
const FAKE = path.join(HERE, "fixtures", "fake-sign.cjs");
const staged = Boolean(bundledEngine() && shippingEngine()) && process.platform === "win32";

function project() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-sign-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: never; images: never; tags: never; scenes: { play: undefined }; ui: {} }
const { scene, game } = define<T>();
export default game({ config: { title: "sign", size: [32, 32], seed: 1, actions: {} }, start: "play",
  scenes: { play: scene("play", {}) }, assets: { sounds: {} } });
`);
  return root;
}

const exportSigned = (root, cmd) => spawnSync(process.execPath, [CLI, "export", "--standalone", "--sign", "--out", "build"],
  { cwd: root, encoding: "utf8", env: { ...process.env, ALPHA_SIGN_CMD: cmd } });
const checkPack = (dir, ...flags) => spawnSync("python", [path.join(REPO, "tools", "check_shipping.py"), "--pack", dir, ...flags],
  { encoding: "utf8" });

test("signCommand puts the quoted file where {file} is, or at the end", () => {
  assert.equal(signCommand("signtool sign /a {file}", "C:\\a b\\x.exe"), 'signtool sign /a "C:\\a b\\x.exe"');
  assert.equal(signCommand("sign-it", "/x"), 'sign-it "/x"');
});

test("signFile without ALPHA_SIGN_CMD says what to set; a failing command is reported", () => {
  assert.match(signFile("x.exe", {}).message, /--sign needs ALPHA_SIGN_CMD/);
  const file = path.join(fs.mkdtempSync(path.join(os.tmpdir(), "alpha-sign-f-")), "e.exe");
  fs.writeFileSync(file, "MZ");
  const failed = signFile(file, { ...process.env, ALPHA_SIGN_CMD: `"${process.execPath}" -e "process.exit(3)" {file}` });
  assert.match(failed.message, /signing command failed/);
});

test("export --sign signs the pack's engine copy only; check_shipping --signed accepts it, rejects tampering",
     { skip: !staged && "needs the staged win32 engines (python tools/stage_engine.py)" }, () => {
  const before = fs.readFileSync(shippingEngine());
  const root = project();
  const ok = exportSigned(root, `"${process.execPath}" "${FAKE}" {file}`);
  assert.equal(ok.status, 0, ok.stderr);
  assert.match(ok.stdout, /\(signed\)/);
  const exe = fs.readFileSync(path.join(root, "build", engineFile(hostTarget())));
  assert.ok(exe.includes("FAKE-PKCS7"));
  assert.deepEqual(fs.readFileSync(shippingEngine()), before);  // the staged engine is untouched
  assert.equal(checkPack(path.join(root, "build"), "--signed").status, 0);
  assert.equal(checkPack(path.join(root, "build")).status, 1);  // unsigned rule: byte identity
  const tampered = exportSigned(root, `"${process.execPath}" "${FAKE}" {file} --tamper`);
  assert.equal(tampered.status, 0, tampered.stderr);
  assert.equal(checkPack(path.join(root, "build"), "--signed").status, 1);
});

test("a failing signing command leaves no pack behind", { skip: !staged && "needs the staged win32 engines" }, () => {
  const root = project();
  const r = exportSigned(root, `"${process.execPath}" -e "process.exit(2)" {file}`);
  assert.equal(r.status, 1);
  assert.match(r.stderr, /signing command failed/);
  assert.equal(fs.existsSync(path.join(root, "build")), false);
});
