// Generates Crates' tile sheet deterministically: `node tools/make-assets.mjs`.
// The file is committed; rerun only to change it. Original art made here, CC0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const hex = (c) => [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16)).concat(255);
const c = { width: 96, height: 16, rgba: Buffer.alloc(96 * 16 * 4) };

function rect(f, x, y, w, h, color) {
  for (let j = y; j < y + h; j++) for (let i = f * 16 + x; i < f * 16 + x + w; i++) c.rgba.set(hex(color), (j * c.width + i) * 4);
}

// Frames: 0 floor, 1 wall, 2 goal, 3 box, 4 box on goal, 5 player.
rect(0, 0, 0, 16, 16, "#3b2a3d"); rect(0, 0, 15, 16, 1, "#33243a");
rect(1, 0, 0, 16, 16, "#8c5e58"); rect(1, 0, 7, 16, 1, "#5b3a37"); rect(1, 7, 0, 1, 7, "#5b3a37"); rect(1, 3, 8, 1, 8, "#5b3a37");
rect(2, 0, 0, 16, 16, "#3b2a3d"); rect(2, 5, 5, 6, 6, "#ffd166"); rect(2, 7, 7, 2, 2, "#3b2a3d");
for (const [f, face] of [[3, "#c98f4a"], [4, "#7fb069"]]) {
  rect(f, 1, 1, 14, 14, "#5b3a1e"); rect(f, 2, 2, 12, 12, face); rect(f, 2, 7, 12, 2, "#5b3a1e"); rect(f, 7, 2, 2, 12, "#5b3a1e");
}
rect(5, 4, 1, 8, 6, "#f2c79b"); rect(5, 3, 0, 10, 2, "#2b59c3"); rect(5, 6, 3, 1, 1, "#1d1d1d"); rect(5, 9, 3, 1, 1, "#1d1d1d");
rect(5, 3, 7, 10, 6, "#ef8354"); rect(5, 4, 13, 3, 3, "#2d3436"); rect(5, 9, 13, 3, 3, "#2d3436");

fs.mkdirSync(path.join(root, "assets/img"), { recursive: true });
fs.writeFileSync(path.join(root, "assets/img/tiles.png"), encodePng(c));
