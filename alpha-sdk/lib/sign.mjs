// `alpha export --standalone --sign`: runs YOUR signing command on the engine copy in the
// pack (never on the SDK's staged engine). The SDK creates and stores no certificates.
//   ALPHA_SIGN_CMD="signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a {file}"
//   ALPHA_SIGN_CMD="codesign --force --options runtime --sign \"Developer ID Application: You\" {file}"
// {file} becomes the quoted path of the executable (appended when the command has no {file}).
import { spawnSync } from "node:child_process";
import fs from "node:fs";

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });

// The shell command line that signs `file`.
export function signCommand(template, file) {
  const quoted = `"${file}"`;
  return template.includes("{file}") ? template.split("{file}").join(quoted) : `${template} ${quoted}`;
}

// Signs `file` in place with $ALPHA_SIGN_CMD. Returns a problem or null.
export function signFile(file, env = process.env) {
  const template = (env.ALPHA_SIGN_CMD ?? "").trim();
  if (!template) {
    return problem("--sign needs ALPHA_SIGN_CMD, your signing command with {file} for the executable, e.g. " +
                   "ALPHA_SIGN_CMD=\"signtool sign /fd SHA256 /a {file}\" (docs: Exporting > Signing)");
  }
  if (!fs.existsSync(file)) return problem(`--sign: no engine executable at ${file} (sign needs --standalone)`, file);
  const run = spawnSync(signCommand(template, file), { shell: true, encoding: "utf8", env });
  if (run.error || run.status !== 0) {
    const output = `${run.stdout ?? ""}${run.stderr ?? ""}`.trim() || String(run.error ?? `exit ${run.status}`);
    return problem(`signing command failed (${signCommand(template, file)}): ${output}`, file);
  }
  return null;
}
