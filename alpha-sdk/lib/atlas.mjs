// Atlas packing for `alpha export`: the game's PNG images and sprite sheets go
// into few RGBA pages so sprites of different images share one texture (one
// draw call per run of same-texture sprites). The engine reads alpha-atlas.json:
//   { "images": { "<file as written in assets>": ["alpha-atlas-0.png", [x, y, w, h]] } }
// Tilesets of maps are not packed (they already are atlases).
import { decodePng, encodePng } from "./png.mjs";

export const MAX_PAGE = 2048;
const GAP = 2;  // transparent pixels between images: no neighbour bleeds in when scaled

/**
 * Shelf packing, tallest first. Returns placements { key, page, x, y } and
 * page sizes; images larger than a page are left out (`skipped`).
 * @param {{ key: string, width: number, height: number }[]} rects
 */
export function packRects(rects, max = MAX_PAGE) {
  const order = [...rects].sort((a, b) => b.height - a.height || b.width - a.width || (a.key < b.key ? -1 : 1));
  const placed = [];
  const skipped = [];
  const pages = [];
  let page = null;
  for (const r of order) {
    if (r.width > max || r.height > max) {
      skipped.push(r.key);
      continue;
    }
    page = page && fits(page, r, max) ? page : newPage(pages);
    if (page.x + r.width > max) shelf(page);
    placed.push({ key: r.key, page: pages.length - 1, x: page.x, y: page.y });
    page.x += r.width + GAP;
    page.shelf = Math.max(page.shelf, r.height);
    page.width = Math.max(page.width, page.x - GAP);
    page.height = Math.max(page.height, page.y + r.height);
  }
  return { placed, skipped, pages: pages.map(({ width, height }) => ({ width, height })) };
}

function newPage(pages) {
  const p = { x: 0, y: 0, shelf: 0, width: 0, height: 0 };
  pages.push(p);
  return p;
}

function shelf(p) {
  p.x = 0;
  p.y += p.shelf + GAP;
  p.shelf = 0;
}

function fits(p, r, max) {
  const y = p.x + r.width > max ? p.y + p.shelf + GAP : p.y;
  return y + r.height <= max;
}

function blit(dst, src, at) {
  for (let row = 0; row < src.height; row++) {
    src.rgba.copy(dst.rgba, ((at.y + row) * dst.width + at.x) * 4, row * src.width * 4, (row + 1) * src.width * 4);
  }
}

/**
 * images: Map(file -> PNG bytes). Returns { pages: [{ file, png }], manifest, packed: [file] };
 * files that are not decodable 8-bit PNGs (or too big) stay out and keep their own file.
 */
export function buildAtlas(images, prefix = "alpha-atlas") {
  const decoded = new Map();
  for (const [file, bytes] of images) {
    const img = decodePng(bytes);
    if (img) decoded.set(file, img);
  }
  const rects = [...decoded].map(([key, img]) => ({ key, width: img.width, height: img.height }));
  const { placed, pages } = packRects(rects);
  const canvases = pages.map(({ width, height }) => ({ width, height, rgba: Buffer.alloc(width * height * 4) }));
  const manifest = { images: {} };
  for (const p of placed) {
    const img = decoded.get(p.key);
    blit(canvases[p.page], img, p);
    manifest.images[p.key] = [`${prefix}-${p.page}.png`, [p.x, p.y, img.width, img.height]];
  }
  return {
    pages: canvases.map((c, i) => ({ file: `${prefix}-${i}.png`, png: encodePng(c) })),
    manifest,
    packed: placed.map((p) => p.key),
  };
}
