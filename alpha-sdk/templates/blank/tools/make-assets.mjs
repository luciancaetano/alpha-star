// Generates the placeholder art: `node tools/make-assets.mjs`. Committed; CC0 (made here).
// Draw your own instead, or edit the pixels below.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const hex = (c) => [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16)).concat(255);
const palette = { a: "#ffccaa", b: "#ff004d", c: "#000000", d: "#29adff" };
const rows = [
  "....bbbbbbbb....", "...bbbbbbbbbb...", "...aaaaaaaaaa...", "...aacaaaacaa...", "...aaaaaaaaaa...",
  "....aaaccaaa....", ".....aaaaaa.....", "...dddddddddd...", "..dddddddddddd..", "..aaddddddddaa..",
  "..aaddddddddaa..", "....dddddddd....", "....dd....dd....", "....dd....dd....", "...ccc....ccc...", "................",
];
const img = { width: 16, height: 16, rgba: Buffer.alloc(16 * 16 * 4) };
rows.forEach((row, y) => [...row].forEach((ch, x) => ch !== "." && img.rgba.set(hex(palette[ch]), (y * 16 + x) * 4)));
fs.mkdirSync(path.join(root, "assets/img"), { recursive: true });
fs.writeFileSync(path.join(root, "assets/img/player.png"), encodePng(img));
