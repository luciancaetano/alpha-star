// Generates the platformer's placeholder art (PNG sprite sheets, tileset) and
// its Tiled map, deterministically: `node tools/make-assets.mjs`. The files are
// committed; rerun only to change them. Original art made here, CC0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");

function canvas(width, height) {
  return { width, height, rgba: Buffer.alloc(width * height * 4) };
}

function hex(color) {
  return [1, 3, 5].map((i) => parseInt(color.slice(i, i + 2), 16)).concat(255);
}

function rect(c, x, y, w, h, color) {
  const px = hex(color);
  for (let j = Math.max(0, y); j < Math.min(c.height, y + h); j++) {
    for (let i = Math.max(0, x); i < Math.min(c.width, x + w); i++) c.rgba.set(px, (j * c.width + i) * 4);
  }
}

function save(file, c) {
  fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
  fs.writeFileSync(path.join(root, file), encodePng(c));
}

// Hero: 16x16 frames. 0 idle, 1-4 run, 5 jump, 6 fall.
function hero() {
  const legs = [[5, 9], [3, 11], [5, 9], [7, 7], [5, 9], [4, 10], [6, 8]];
  const c = canvas(16 * legs.length, 16);
  legs.forEach(([left, right], f) => {
    const x = f * 16;
    const lift = f === 5 ? 2 : 0;
    rect(c, x + 4, 1, 8, 6, "#f2c79b");          // head
    rect(c, x + 8, 3, 2, 2, "#1d1d1d");          // eye (facing right)
    rect(c, x + 3, 0, 10, 2, "#c0392b");         // cap
    rect(c, x + 4, 7, 8, 5, "#2e86de");          // body
    rect(c, x + left, 12 - lift, 3, 4, "#34495e");
    rect(c, x + right, 12 - lift, 3, 4, "#34495e");
  });
  save("assets/img/hero.png", c);
}

// Coin: 8x8, 4 frames of a spin.
function coin() {
  const c = canvas(32, 8);
  [8, 6, 2, 6].forEach((w, f) => {
    rect(c, f * 8 + (8 - w) / 2, 0, w, 8, "#f1c40f");
    rect(c, f * 8 + (8 - w) / 2, 1, Math.max(1, w / 2), 6, "#f9e79f");
  });
  save("assets/img/coin.png", c);
}

// Tileset: 16x16 tiles. 1 grass, 2 dirt, 3 brick, 4 bush, 5 flower.
function tiles() {
  const c = canvas(80, 16);
  rect(c, 0, 0, 16, 16, "#8e5b34");
  rect(c, 0, 0, 16, 5, "#4caf50");
  rect(c, 16, 0, 16, 16, "#8e5b34");
  rect(c, 20, 6, 2, 2, "#6d4425");
  rect(c, 32, 0, 16, 16, "#b5523b");
  for (const [x, y, w] of [[32, 7, 16], [32, 15, 16], [39, 0, 1], [35, 8, 1], [43, 8, 1]]) rect(c, x, y, w, 1, "#7b3426");
  rect(c, 50, 6, 12, 10, "#2e7d32");
  rect(c, 53, 3, 6, 4, "#2e7d32");
  rect(c, 71, 8, 2, 8, "#2e7d32");
  rect(c, 69, 4, 6, 4, "#e84393");
  save("assets/tiles/tiles.png", c);
}

const W = 60;
const H = 12;

function cells(fill) {
  const data = new Array(W * H).fill(0);
  fill((x, y, gid) => { data[y * W + x] = gid; });
  return data;
}

function map() {
  const walls = cells((set) => {
    for (let x = 0; x < W; x++) {
      if (x >= 24 && x < 27) continue;           // a pit
      set(x, 10, 1);
      set(x, 11, 2);
    }
    for (let x = 12; x < 16; x++) set(x, 7, 3);  // brick platforms
    for (let x = 30; x < 34; x++) set(x, 6, 3);
    for (let x = 40; x < 43; x++) set(x, 8, 3);
    for (let y = 0; y < 10; y++) { set(0, y, 3); set(W - 1, y, 3); }  // side walls
  });
  const deco = cells((set) => {
    for (const x of [5, 18, 37, 50]) set(x, 9, 4);
    for (const x of [8, 21, 45, 53]) set(x, 9, 5);
  });
  const coins = [[100, 150], [130, 150], [216, 100], [500, 84], [530, 84], [660, 116], [800, 150], [880, 150]];
  const objects = [{ id: 1, name: "player", type: "spawn", x: 40, y: 140, point: true }]
    .concat(coins.map(([x, y], i) => ({ id: i + 2, name: "", type: "coin", x, y, point: true })));
  const layer = (id, name, data) => ({ id, type: "tilelayer", name, width: W, height: H, x: 0, y: 0, opacity: 1, visible: true, data });
  const tmj = {
    type: "map", version: "1.10", tiledversion: "1.10.2", orientation: "orthogonal", renderorder: "right-down",
    infinite: false, width: W, height: H, tilewidth: 16, tileheight: 16, nextlayerid: 4, nextobjectid: objects.length + 1,
    tilesets: [{ firstgid: 1, source: "tiles.tsj" }],
    layers: [layer(1, "deco", deco), layer(2, "walls", walls),
      { id: 3, type: "objectgroup", name: "spawns", x: 0, y: 0, opacity: 1, visible: true, draworder: "topdown", objects }],
  };
  const tsj = { type: "tileset", version: "1.10", name: "tiles", image: "../tiles/tiles.png", imagewidth: 80, imageheight: 16,
    tilewidth: 16, tileheight: 16, tilecount: 5, columns: 5, margin: 0, spacing: 0 };
  fs.mkdirSync(path.join(root, "assets/maps"), { recursive: true });
  fs.writeFileSync(path.join(root, "assets/maps/level1.tmj"), JSON.stringify(tmj, null, 1));
  fs.writeFileSync(path.join(root, "assets/maps/tiles.tsj"), JSON.stringify(tsj, null, 1));
}

hero();
coin();
tiles();
map();
console.log("wrote assets/img, assets/tiles, assets/maps");
