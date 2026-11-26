"""Benchmarks: scripted scenarios run headlessly on the release engine with fixed seeds.

Each scenario in bench/*.js runs `--frames N --hidden --perf out.json` (one fixed step per
frame); the engine keeps the newest 600 frames, so the first N - 600 are warm-up.
Prints ms/frame p50 / p95 / max per subsystem. Not part of the gate.

Usage: python tools/bench.py [--preset release] [--frames 720] [--only NAME] [--md]
"""
import argparse
import json
import shutil
import struct
import subprocess
import sys
from pathlib import Path

import harness_config as cfg
import resource_guard
import toolchain

SECTIONS = ["script", "timers", "physics", "particles", "animation", "ui", "render", "audio"]


def bmp(path, w, h, rgba):
    """Uncompressed 32-bit BMP of one color (bench sprites and tiles)."""
    pixels = bytes([rgba[2], rgba[1], rgba[0], rgba[3]]) * (w * h)
    header = struct.pack("<2sIHHI", b"BM", 54 + len(pixels), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 32, 0, len(pixels), 2835, 2835, 0, 0)
    path.write_bytes(header + info + pixels)


def tilemap(out, size=500):
    bmp(out / "tiles.bmp", 16, 8, (90, 160, 90, 255))
    tileset = {"uid": 1, "identifier": "Tiles", "relPath": "tiles.bmp", "pxWid": 16, "pxHei": 8, "tileGridSize": 8,
               "spacing": 0, "padding": 0, "__cWid": 2, "__cHei": 1}
    tiles = [{"px": [x * 8, y * 8], "src": [((x + y) % 2) * 8, 0], "t": (x + y) % 2, "f": 0}
             for y in range(size) for x in range(size)]
    layer = {"__identifier": "Ground", "__type": "Tiles", "__cWid": size, "__cHei": size, "__gridSize": 8,
             "iid": "G", "layerDefUid": 1, "__tilesetDefUid": 1, "gridTiles": tiles}
    level = {"identifier": "Big", "iid": "L", "worldX": 0, "worldY": 0, "pxWid": size * 8, "pxHei": size * 8,
             "layerInstances": [layer]}
    project = {"jsonVersion": "1.5.3", "worldLayout": "Free", "defs": {"tilesets": [tileset], "layers": [], "entities": []},
               "levels": [level]}
    (out / "big.ldtk").write_text(json.dumps(project), encoding="utf-8")


def stage(name, root):
    out = root / name
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    shutil.copy(cfg.ROOT / "bench" / f"{name}.js", out / "main.js")
    bmp(out / "dot.bmp", 4, 4, (255, 255, 255, 255))
    if name == "tilemap":
        tilemap(out)
    if name == "ui":
        items = "".join(f'<p id="e{i}" style="display: inline-block; width: 30px;">0</p>' for i in range(200))
        (out / "hud.rml").write_text(f"<rml><head></head><body>{items}</body></rml>", encoding="utf-8")
    return out


def run(exe, pack, frames):
    report = pack / "perf.json"
    cmd = [str(exe), str(pack), "--frames", str(frames), "--hidden", "--perf", str(report)]
    done = subprocess.run(cmd, check=True, capture_output=True, timeout=600, text=True, encoding="utf-8")
    if "Error" in done.stderr or "script ran" in done.stderr:  # a paused game measures nothing
        raise SystemExit(f"bench {pack.name}: the game stopped:\n{done.stderr}")
    return json.loads(report.read_text(encoding="utf-8"))


def row(name, r):
    p = r["perf"]
    cells = [f"{p[s]['p50']:.2f}/{p[s]['p95']:.2f}/{p[s]['max']:.2f}" for s in ["total", *SECTIONS]]
    return [name, *cells, str(r["drawCalls"]), str(r["vertices"]), str(r["entities"]), f"{r['jsBytes'] / 1e6:.1f}"]


def main():
    resource_guard.limit()
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="release")
    ap.add_argument("--frames", type=int, default=720)
    ap.add_argument("--only")
    ap.add_argument("--md", action="store_true", help="markdown table (docs/PERF.md)")
    a = ap.parse_args()
    exe = toolchain.built_exe(a.preset)
    names = sorted(p.stem for p in (cfg.ROOT / "bench").glob("*.js") if not a.only or p.stem == a.only)
    head = ["scenario", "total", *SECTIONS, "draws", "verts", "entities", "JS MB"]
    rows = [row(n, run(exe, stage(n, cfg.ROOT / "build" / "bench"), a.frames)) for n in names]
    if a.md:
        print("| " + " | ".join(head) + " |\n|" + "---|" * len(head))
        for r in rows:
            print("| " + " | ".join(r) + " |")
    else:
        print("ms p50/p95/max per frame")
        for r in [head, *rows]:
            print("  ".join(c.ljust(18 if i else 14) for i, c in enumerate(r)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
