import { test } from "node:test";
import assert from "node:assert/strict";
import zlib from "node:zlib";
import { buildAtlas, packRects } from "../lib/atlas.mjs";
import { decodePng, encodePng } from "../lib/png.mjs";

test("PNG round trip keeps every RGBA pixel", () => {
  const rgba = Buffer.from(Array.from({ length: 5 * 3 * 4 }, (_, i) => (i * 37) & 0xff));
  const back = decodePng(encodePng({ width: 5, height: 3, rgba }));
  assert.equal(back.width, 5);
  assert.deepEqual([...back.rgba], [...rgba]);
});

test("decodePng undoes row filters and expands palette images with transparency", () => {
  // 2x2 palette image: index rows [0, 1] (filter Sub: second byte stored as delta) and [1, 0] (filter Up).
  const rows = Buffer.from([1, 0, 1, 2, 1, 0xff]);
  const chunk = (type, data) => {
    const head = Buffer.alloc(8);
    head.writeUInt32BE(data.length);
    head.write(type, 4, "latin1");
    return Buffer.concat([head, data, Buffer.alloc(4)]);  // CRC is not checked by the decoder
  };
  const ihdr = Buffer.from([0, 0, 0, 2, 0, 0, 0, 2, 8, 3, 0, 0, 0]);
  const png = Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr),
    chunk("PLTE", Buffer.from([10, 20, 30, 40, 50, 60])), chunk("tRNS", Buffer.from([0])),
    chunk("IDAT", zlib.deflateSync(rows)), chunk("IEND", Buffer.alloc(0))]);
  const img = decodePng(png);
  assert.deepEqual([...img.rgba], [10, 20, 30, 0, 40, 50, 60, 255, 40, 50, 60, 255, 10, 20, 30, 0]);
});

test("decodePng returns null for what it does not handle (16-bit, not a PNG)", () => {
  assert.equal(decodePng(Buffer.from("not a png")), null);
  const ihdr16 = encodePng({ width: 1, height: 1, rgba: Buffer.alloc(4) });
  ihdr16[24] = 16;  // bit depth byte of IHDR
  assert.equal(decodePng(ihdr16), null);
});

test("packRects places every rect inside a page without overlaps and skips oversize ones", () => {
  const rects = Array.from({ length: 60 }, (_, i) => ({ key: `r${i}`, width: 10 + (i * 7) % 50, height: 5 + (i * 13) % 40 }));
  rects.push({ key: "huge", width: 300, height: 10 });
  const { placed, skipped, pages } = packRects(rects, 256);
  assert.deepEqual(skipped, ["huge"]);
  assert.equal(placed.length, 60);
  const size = Object.fromEntries(rects.map((r) => [r.key, r]));
  for (const a of placed) {
    const ra = size[a.key];
    assert.ok(a.x + ra.width <= pages[a.page].width && a.y + ra.height <= pages[a.page].height, a.key);
    for (const b of placed) {
      if (a === b || a.page !== b.page) continue;
      const rb = size[b.key];
      const apart = a.x + ra.width <= b.x || b.x + rb.width <= a.x || a.y + ra.height <= b.y || b.y + rb.height <= a.y;
      assert.ok(apart, `${a.key} overlaps ${b.key}`);
    }
  }
});

test("buildAtlas leaves undecodable files out and is deterministic", () => {
  const red = encodePng({ width: 2, height: 2, rgba: Buffer.alloc(16, 200) });
  const images = new Map([["a.png", red], ["b.png", red], ["c.png", Buffer.from("JPEG?")]]);
  const one = buildAtlas(images);
  assert.deepEqual(one.packed.sort(), ["a.png", "b.png"]);
  assert.equal(one.pages.length, 1);
  assert.deepEqual(buildAtlas(images).pages[0].png, one.pages[0].png);
});

test("buildAtlas extrudes each image's edge pixels into the gap, so edge sampling never reads transparency", () => {
  const solid = (v) => encodePng({ width: 2, height: 2, rgba: Buffer.alloc(16, v) });
  const { pages, manifest } = buildAtlas(new Map([["a.png", solid(100)], ["b.png", solid(200)]]));
  const page = decodePng(pages[0].png);
  const at = (x, y) => [...page.rgba.subarray((y * page.width + x) * 4, (y * page.width + x) * 4 + 4)];
  const [, [ax]] = manifest.images["a.png"];
  const [, [bx]] = manifest.images["b.png"];
  const [left, right] = ax < bx ? [100, 200] : [200, 100];
  const gap = Math.min(ax, bx) + 2;
  assert.deepEqual(at(gap, 0), [left, left, left, left]);
  assert.deepEqual(at(gap + 1, 1), [right, right, right, right]);
});
