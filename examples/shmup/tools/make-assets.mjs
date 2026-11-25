// Generates Starfighter's art and music loop deterministically: `node tools/make-assets.mjs`.
// The files are committed; rerun only to change them. Original work made here, CC0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const hex = (c) => [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16)).concat(255);

function save(file, bytes) {
  fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
  fs.writeFileSync(path.join(root, file), bytes);
}

// A 16x16 sprite from rows of text: each character is a palette key ('.' = transparent).
function sprite(rows, palette) {
  const c = { width: 16, height: 16, rgba: Buffer.alloc(16 * 16 * 4) };
  rows.forEach((row, y) => [...row].forEach((ch, x) => ch !== "." && c.rgba.set(hex(palette[ch]), (y * 16 + x) * 4)));
  return encodePng(c);
}

save("assets/img/ship.png", sprite([
  ".......aa.......", "......abba......", "......abba......", ".....abccba.....", ".....abccba.....",
  "....aabbbbaa....", "...aabbbbbbaa...", "..aabbbaabbbaa..", ".aabbbaaaabbbaa.", "aabbba....abbbaa",
  "abbba......abbba", "aaaa..dddd..aaaa", "......dede......", ".......ee.......", "................", "................",
], { a: "#2b59c3", b: "#9bf6ff", c: "#ffffff", d: "#ef476f", e: "#ffd166" }));

save("assets/img/enemy.png", sprite([
  "................", "...a........a...", "....a......a....", "...aaaaaaaaaa...", "..aabbaaaabbaa..",
  ".aaabbaaaabbaaa.", "aaaaaaaaaaaaaaaa", "a.aaaaaaaaaaaa.a", "a.a..........a.a", "...aa......aa...",
  "................", "................", "................", "................", "................", "................",
], { a: "#7bd389", b: "#0b0d1a" }));

// Music: 16 beats at 150 bpm, square-wave arpeggio over a triangle bass, 16-bit mono WAV.
function wav(seconds, rate, sample) {
  const n = Math.round(seconds * rate);
  const b = Buffer.alloc(44 + n * 2);
  b.write("RIFF", 0); b.writeUInt32LE(36 + n * 2, 4); b.write("WAVEfmt ", 8);
  b.writeUInt32LE(16, 16); b.writeUInt16LE(1, 20); b.writeUInt16LE(1, 22);
  b.writeUInt32LE(rate, 24); b.writeUInt32LE(rate * 2, 28); b.writeUInt16LE(2, 32); b.writeUInt16LE(16, 34);
  b.write("data", 36); b.writeUInt32LE(n * 2, 40);
  for (let i = 0; i < n; i++) b.writeInt16LE(Math.round(Math.max(-1, Math.min(1, sample(i / rate))) * 12000), 44 + i * 2);
  return b;
}

const beat = 60 / 150;
const roots = [110, 110, 87.31, 98];                // A2 A2 F2 G2, four beats each
const arp = [1, 1.5, 2, 1.5];                       // root, fifth, octave, fifth
save("assets/music/theme.wav", wav(16 * beat, 22050, (t) => {
  const b = Math.floor(t / beat);
  const root = roots[Math.floor(b / 4) % 4];
  const sub = Math.floor(t / (beat / 4)) % 4;
  const f = root * 4 * arp[sub];
  const inNote = (t % (beat / 4)) / (beat / 4);
  const lead = (Math.floor(t * f * 2) % 2 ? 0.25 : -0.25) * (1 - inNote * 0.8);
  const bass = (2 * Math.abs(2 * ((t * root) % 1) - 1) - 1) * 0.45;
  return lead + bass;
}));
