// Generates Arena's placeholder art deterministically: `node tools/make-assets.mjs`.
// The files are committed; rerun only to change them. Original art made here, CC0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const hex = (c) => [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16)).concat(255);

function rect(c, x, y, w, h, color) {
  for (let j = y; j < y + h; j++) for (let i = x; i < x + w; i++) c.rgba.set(hex(color), (j * c.width + i) * 4);
}

// Hero seen from above, 16x16 frames: 0 standing, 1-2 stepping (feet apart).
const c = { width: 48, height: 16, rgba: Buffer.alloc(48 * 16 * 4) };
[[0, 0], [-2, 2], [2, -2]].forEach(([l, r], f) => {
  const x = f * 16;
  rect(c, x + 4, 11 + l, 3, 3, "#2d3436");   // left foot
  rect(c, x + 9, 11 + r, 3, 3, "#2d3436");   // right foot
  rect(c, x + 3, 5, 10, 7, "#27ae60");       // body
  rect(c, x + 5, 2, 6, 5, "#f2c79b");        // head
  rect(c, x + 9, 3, 1, 1, "#1d1d1d");        // eye (facing right)
  rect(c, x + 12, 7, 3, 2, "#95a5a6");       // blaster
});
fs.mkdirSync(path.join(root, "assets/img"), { recursive: true });
fs.writeFileSync(path.join(root, "assets/img/hero.png"), encodePng(c));
