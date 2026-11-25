// Minimal PNG codec (node:zlib only): decodes 8-bit non-interlaced PNGs of
// every color type to RGBA, encodes RGBA. Anything else (16-bit, interlaced)
// returns null from decodePng and is left out of atlases (copied as is).
import zlib from "node:zlib";

const SIGNATURE = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);
const CHANNELS = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 };

const CRC_TABLE = Array.from({ length: 256 }, (_, n) => {
  let c = n;
  for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
  return c >>> 0;
});

function crc32(buf) {
  let c = 0xffffffff;
  for (const b of buf) c = CRC_TABLE[(c ^ b) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunks(buf) {
  const out = [];
  for (let at = 8; at + 8 <= buf.length;) {
    const length = buf.readUInt32BE(at);
    out.push({ type: buf.toString("latin1", at + 4, at + 8), data: buf.subarray(at + 8, at + 8 + length) });
    at += 12 + length;
  }
  return out;
}

function paeth(a, b, c) {
  const p = a + b - c;
  const pa = Math.abs(p - a);
  const pb = Math.abs(p - b);
  const pc = Math.abs(p - c);
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

// Undoes the per-row filters in place; returns the raw scanlines (no filter bytes).
function unfilter(data, width, height, bpp) {
  const stride = width * bpp;
  const out = Buffer.alloc(stride * height);
  for (let y = 0; y < height; y++) {
    const type = data[y * (stride + 1)];
    const row = data.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1));
    for (let x = 0; x < stride; x++) {
      const a = x >= bpp ? out[y * stride + x - bpp] : 0;
      const b = y > 0 ? out[(y - 1) * stride + x] : 0;
      const c = x >= bpp && y > 0 ? out[(y - 1) * stride + x - bpp] : 0;
      const pred = [0, a, b, (a + b) >> 1, paeth(a, b, c)][type];
      out[y * stride + x] = (row[x] + pred) & 0xff;
    }
  }
  return out;
}

function toRgba(raw, header, palette, trns) {
  const { width, height, colorType } = header;
  const n = CHANNELS[colorType];
  const rgba = Buffer.alloc(width * height * 4);
  for (let i = 0; i < width * height; i++) {
    const p = raw.subarray(i * n, i * n + n);
    let px;
    if (colorType === 3) px = [...palette.subarray(p[0] * 3, p[0] * 3 + 3), trns?.[p[0]] ?? 255];
    else if (colorType === 0) px = [p[0], p[0], p[0], trns && trns.readUInt16BE(0) === p[0] ? 0 : 255];
    else if (colorType === 4) px = [p[0], p[0], p[0], p[1]];
    else if (colorType === 2) px = [p[0], p[1], p[2], trns && trns.readUInt16BE(0) === p[0] && trns.readUInt16BE(2) === p[1] && trns.readUInt16BE(4) === p[2] ? 0 : 255];
    else px = p;
    rgba.set(px, i * 4);
  }
  return rgba;
}

/** @returns {{ width: number, height: number, rgba: Buffer } | null} null for PNGs this codec does not handle. */
export function decodePng(buf) {
  if (buf.length < 8 || !buf.subarray(0, 8).equals(SIGNATURE)) return null;
  const all = chunks(buf);
  const ihdr = all.find((c) => c.type === "IHDR")?.data;
  if (!ihdr) return null;
  const header = { width: ihdr.readUInt32BE(0), height: ihdr.readUInt32BE(4), depth: ihdr[8], colorType: ihdr[9], interlace: ihdr[12] };
  if (header.depth !== 8 || header.interlace !== 0 || !(header.colorType in CHANNELS)) return null;
  const idat = Buffer.concat(all.filter((c) => c.type === "IDAT").map((c) => c.data));
  const raw = unfilter(zlib.inflateSync(idat), header.width, header.height, CHANNELS[header.colorType]);
  const palette = all.find((c) => c.type === "PLTE")?.data;
  const trns = all.find((c) => c.type === "tRNS")?.data;
  return { width: header.width, height: header.height, rgba: toRgba(raw, header, palette, trns) };
}

function chunk(type, data) {
  const head = Buffer.alloc(8);
  head.writeUInt32BE(data.length, 0);
  head.write(type, 4, "latin1");
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(Buffer.concat([head.subarray(4), data])), 0);
  return Buffer.concat([head, data, crc]);
}

/** RGBA8 pixels -> PNG file bytes (filter 0 on every row). */
export function encodePng({ width, height, rgba }) {
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr.set([8, 6, 0, 0, 0], 8);
  const rows = Buffer.alloc((width * 4 + 1) * height);
  for (let y = 0; y < height; y++) rgba.copy(rows, y * (width * 4 + 1) + 1, y * width * 4, (y + 1) * width * 4);
  return Buffer.concat([SIGNATURE, chunk("IHDR", ihdr), chunk("IDAT", zlib.deflateSync(rows)), chunk("IEND", Buffer.alloc(0))]);
}
