"""Turns the Codex-generated art in art/raw/ into game assets: `python -I tools/process-art.py`.

Each raw image is a row (or grid) of sprites on flat magenta. Sprites are found by splitting
at the widest empty gaps (the expected count is known), keyed out, scaled to a fixed height
and packed into fixed-size sheet cells. Icons also become two BMFont fonts (16 px HUD, 32 px cards).
"""
import colorsys
import os
import sys
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
RAW = os.path.join(ROOT, "art", "raw")
IMG = os.path.join(ROOT, "assets", "img")
FONTS = os.path.join(ROOT, "assets", "fonts")
UI = os.path.join(ROOT, "assets", "ui")


def keyed(path):
    im = Image.open(os.path.join(RAW, path)).convert("RGBA")
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, _ = px[x, y]
            if r > 140 and b > 140 and g < 110 and abs(r - b) < 90:
                px[x, y] = (0, 0, 0, 0)
    return im


def runs(profile, n):
    """Splits a 1-D occupancy profile into n spans at the n-1 widest interior gaps."""
    filled = [i for i, v in enumerate(profile) if v]
    lo, hi = filled[0], filled[-1]
    gaps, start = [], None
    for i in range(lo, hi + 1):
        if not profile[i] and start is None:
            start = i
        if profile[i] and start is not None:
            gaps.append((i - start, start, i))
            start = None
    cuts = sorted(sorted(gaps, reverse=True)[: n - 1], key=lambda g: g[1])
    spans, at = [], lo
    for _, s, e in cuts:
        spans.append((at, s))
        at = e
    spans.append((at, hi + 1))
    return spans


def occupancy(im, axis):
    a = im.getchannel("A").point(lambda v: 255 if v > 40 else 0)
    w, h = im.size
    if axis == 0:
        return [any(a.getpixel((x, y)) for y in range(0, h, 2)) for x in range(w)]
    return [any(a.getpixel((x, y)) for x in range(0, w, 2)) for y in range(h)]


def sprites(path, cols, rows=1):
    im = keyed(path)
    out = []
    for (y0, y1) in runs(occupancy(im, 1), rows):
        band = im.crop((0, y0, im.width, y1))
        for (x0, x1) in runs(occupancy(band, 0), cols):
            piece = band.crop((x0, 0, x1, band.height))
            out.append(piece.crop(piece.getbbox()))
    return out


def shrink(im, scale):
    """Premultiplied box downscale, then hard alpha: crisp pixel art with no magenta fringe."""
    w, h = max(1, round(im.width * scale)), max(1, round(im.height * scale))
    r, g, b, a = im.split()
    pm = Image.merge("RGBA", [Image.composite(c, Image.new("L", im.size, 0), a) for c in (r, g, b)] + [a])
    small = pm.resize((w, h), Image.BOX)
    px = small.load()
    for y in range(h):
        for x in range(w):
            cr, cg, cb, ca = px[x, y]
            px[x, y] = (0, 0, 0, 0) if ca < 110 else (min(255, cr * 255 // ca), min(255, cg * 255 // ca), min(255, cb * 255 // ca), 255)
    return small


def sheet(name, pieces, cell, height, bottom=True, heights=None):
    tallest = max(p.height for p in pieces)
    out = Image.new("RGBA", (cell[0] * len(pieces), cell[1]), (0, 0, 0, 0))
    for i, p in enumerate(pieces):
        target = heights[i] if heights else height
        scale = target / (p.height if heights else tallest)
        s = shrink(p, min(scale, cell[0] / p.width, cell[1] / p.height))
        x = i * cell[0] + (cell[0] - s.width) // 2
        y = cell[1] - s.height - 1 if bottom else (cell[1] - s.height) // 2
        out.alpha_composite(s, (x, y))
    out.save(os.path.join(IMG, f"{name}.png"))
    return out


def font(name, glyphs, size):
    """glyphs: list of (char, image). Writes an AngelCode text BMFont and its page."""
    per_row = 16
    page = Image.new("RGBA", (per_row * (size + 2), ((len(glyphs) + per_row - 1) // per_row) * (size + 2)), (0, 0, 0, 0))
    lines = []
    for i, (ch, g) in enumerate(glyphs):
        x, y = (i % per_row) * (size + 2), (i // per_row) * (size + 2)
        page.alpha_composite(g, (x, y))
        yoff = size - g.height if g.height < size else 0
        lines.append(f"char id={ord(ch)} x={x} y={y} width={g.width} height={g.height} xoffset=0 yoffset={yoff // 2 if ch in '#-' else yoff} xadvance={g.width + (0 if ch in '#-' else 1)} page=0 chnl=15")
    page.save(os.path.join(FONTS, f"{name}.png"))
    head = [f'info face="{name}" size={size} bold=0 italic=0 charset="" unicode=1 stretchH=100 smooth=0 aa=1 padding=0,0,0,0 spacing=1,1',
            f"common lineHeight={size} base={size} scaleW={page.width} scaleH={page.height} pages=1 packed=0",
            f'page id=0 file="{name}.png"', f"chars count={len(glyphs) + 1}",
            f"char id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 xadvance={size // 2} page=0 chnl=15"]
    with open(os.path.join(FONTS, f"{name}.fnt"), "w", newline="\n") as f:
        f.write("\n".join(head + lines) + "\n")


def fitted(p, size):
    s = shrink(p, min(size / p.width, size / p.height))
    cell = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    cell.alpha_composite(s, ((size - s.width) // 2, (size - s.height) // 2))
    return cell


def bar(color, size):
    g = Image.new("RGBA", (max(2, size // 4), size // 2), (26, 16, 32, 255))
    inner = Image.new("RGBA", (g.width - 1, g.height - 2), color)
    g.paste(inner, (0, 1))
    return g


def variant(im, hue=0, sat=1.0, val=1.0):
    """Recolors a sprite: hue rotated by `hue` degrees, saturation and brightness scaled."""
    out = im.copy()
    px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            r, g, b, a = px[x, y]
            if not a:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            r, g, b = colorsys.hsv_to_rgb((h + hue / 360) % 1, min(1, s * sat), min(1, v * val))
            px[x, y] = (round(r * 255), round(g * 255), round(b * 255), a)
    return out


# Extra heroes: recolored copies of the drawn ones (sheet name, source, hue, saturation, brightness).
HERO_VARIANTS = [("ash", "wick", 0, 0.25, 0.8), ("vesper", "taper", 225, 1.1, 1.0), ("tallow", "lumen", 120, 1.0, 1.05)]


def tinted_glyphs(bases):
    """Glyph g (0-9 weapons A-J, 10-16 passives a-g) in hue step h (1-7, 45 degrees each) is
    chr(0x100 + g * 8 + h): src/data/glyphs.ts computes the same code."""
    return [(chr(0x100 + g * 8 + h), variant(im, h * 45, 1.15)) for g, im in enumerate(bases) for h in range(1, 8)]


def icons(heroes, items):
    weapons = sprites("icons_weapons.png", 5, 2)
    passives = sprites("icons_passives.png", 7)
    pairs = list(zip("ABCDEFGHIJ", weapons)) + list(zip("abcdefg", passives))
    pairs += [("$", items[3]), ("%", items[4])] + list(zip("123456", heroes))
    pairs += tinted_glyphs(weapons + passives)
    for size in (16, 32):
        glyphs = [(c, fitted(p, size)) for c, p in pairs]
        slot = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        slot.paste(Image.new("RGBA", (size - 4, size - 4), (60, 40, 70, 160)), (2, 2))
        glyphs += [("*", slot), ("#", bar((120, 220, 255, 255), size)), ("-", bar((50, 36, 64, 255), size))]
        font("icons" if size == 16 else "bigicons", glyphs, size)


def crisp(im, name):
    """UI images: true pixel size, then x4 nearest. The engine filters UI images linearly,
    so a pre-enlarged copy stays sharp when drawn at 1-4x. Returns the native size."""
    im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(UI, name))
    return im.size


def ui_art():
    card, _panel, button = sprites("ui_frames.png", 3)
    sizes = {
        "card.png": crisp(shrink(card, 104 / card.height), "card.png"),
        "button.png": crisp(shrink(button, 18 / button.height), "button.png"),
    }
    logo = sprites("logo.png", 1)[0]
    sizes["logo.png"] = crisp(shrink(logo, 200 / logo.width), "logo.png")
    print("ui native sizes", sizes)


def main():
    for d in (IMG, FONTS, UI):
        os.makedirs(d, exist_ok=True)
    heroes = []
    for name in ("wick", "taper", "lumen"):
        frames = sprites(f"hero_{name}.png", 4)
        sheet(name, frames, (32, 32), 29)
        heroes.append(frames[0])
        for name2, src, hue, sat, val in HERO_VARIANTS:
            if src == name:
                sheet(name2, [variant(f, hue, sat, val) for f in frames], (32, 32), 29)
    heroes += [variant(heroes[("wick", "taper", "lumen").index(src)], hue, sat, val) for _, src, hue, sat, val in HERO_VARIANTS]
    for name, raw, cell, h in [("moth", "moth.png", 24, 18), ("shade", "shade.png", 24, 20), ("wisp", "wisp.png", 24, 20),
                               ("ghoul", "ghoul.png", 32, 26), ("gargoyle", "gargoyle.png", 48, 34), ("queen", "moth_queen.png", 96, 70),
                               ("snuffer", "snuffer.png", 72, 66)]:
        sheet(name, sprites(raw, 2), (cell, cell), h, bottom=False)
    items = sprites("pickups.png", 8)
    sheet("items", items, (14, 14), 0, bottom=False, heights=[8, 9, 12, 10, 10, 11, 12, 10])
    sheet("props", sprites("props.png", 9), (32, 32), 0, heights=[16, 18, 28, 28, 22, 26, 16, 14, 26])
    sheet("fx", sprites("fx.png", 6), (32, 32), 0, bottom=False, heights=[8, 10, 30, 14, 26, 30])
    ground = Image.open(os.path.join(RAW, "ground.png")).convert("RGB").resize((128, 128), Image.LANCZOS)
    ground.save(os.path.join(IMG, "ground.png"))
    icons(heroes, items)
    ui_art()
    print("assets written")


if __name__ == "__main__":
    sys.exit(main())
