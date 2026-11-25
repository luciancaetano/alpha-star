// `alpha run --watch`: re-export the pack whenever the project's sources or
// assets change. The engine (dev/release) watches the pack and hot-reloads
// the running game; a failed export (type errors) writes nothing, so the
// running game keeps working until the code compiles again.
import fs from "node:fs";
import path from "node:path";
import { exportPack } from "./pack.mjs";

export const DEBOUNCE_MS = 200;
// Never watched: outputs and tooling (a change there is not a game change).
const IGNORED = new Set(["node_modules", ".git", "build", "dist"]);

function ignored(root, out, file) {
  if (!file) return false;
  const full = path.resolve(root, file);
  if (full === out || full.startsWith(out + path.sep)) return true;
  return IGNORED.has(file.split(/[\\/]/)[0]);
}

/**
 * Watches `root` (recursively) and re-exports `entry` into `out` once the
 * project has been quiet for `debounceMs` after a change. `onExport(result)`
 * gets exportPack's { ok, problems, files } after each export. Returns
 * { close }. Exports never overlap: a change during an export queues one more.
 */
export function watchProject({ root, entry, out, onExport, debounceMs = DEBOUNCE_MS }) {
  const outDir = path.resolve(out);
  let timer = null;
  let running = false;
  let again = false;
  const run = async () => {
    if (running) return void (again = true);
    running = true;
    try {
      onExport(await exportPack({ root, entry, out: outDir }));
    } finally {
      running = false;
      if (again) {
        again = false;
        schedule();
      }
    }
  };
  const schedule = () => {
    clearTimeout(timer);
    timer = setTimeout(run, debounceMs);
  };
  const watcher = fs.watch(root, { recursive: true }, (_event, file) => {
    if (!ignored(root, outDir, file)) schedule();
  });
  return {
    close() {
      clearTimeout(timer);
      watcher.close();
    },
  };
}
