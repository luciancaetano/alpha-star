// Records the showcase run (V on the title) frame by frame, with no window: the release engine runs
// `--hidden` under the control channel, steps 2 fixed steps per video frame (30 fps) and saves PNGs.
// Usage (from examples/survivors, after `npx alpha export`): node tools/record-showcase.mjs <outDir> [seconds]
// Then tools/encode-showcase.py turns the PNGs into the docs video.
import fs from "node:fs";
import path from "node:path";
import { GameClient } from "../node_modules/@alpha-star/sdk/lib/control.mjs";   // not a public export
import { bundledEngine } from "@alpha-star/sdk/engine";

const [outDir, secondsArg] = process.argv.slice(2);
if (!outDir) throw new Error("usage: node tools/record-showcase.mjs <outDir> [seconds]");
const frames = Math.round(Number(secondsArg ?? 20) * 30);
fs.mkdirSync(outDir, { recursive: true });

const game = await GameClient.start({ engine: bundledEngine(), pack: path.resolve("dist"), hidden: true, cwd: process.cwd() });
try {
  await game.request("pause");
  await game.request("step", { frames: 10 });
  await game.request("press", { action: "showcase", player: 1 });
  await game.request("step", { frames: 90 });   // the horde gathers before the first frame
  for (let i = 0; i < frames; i++) {
    await game.request("step", { frames: 2 });
    const shot = await game.request("screenshot");
    fs.writeFileSync(path.join(outDir, `f${String(i).padStart(5, "0")}.png`), Buffer.from(shot.png, "base64"));
    if (i % 60 === 0) console.log(`frame ${i}/${frames} (${shot.width}x${shot.height})`);
  }
} finally {
  await game.stop();
}
