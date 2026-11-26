"""Writes alpha-sdk/notices/*.txt: the license notices of everything linked into
the engine a pack ships with (the shipping flavor), copied verbatim from the
sources CMake fetched (build/<preset>/_deps). `alpha export` joins them into
THIRD_PARTY_NOTICES.txt in every pack. Dear ImGui (dev tools) and doctest
(tests) never reach a player, so they are left out.

usage: python tools/make_notices.py [--deps build/dev/_deps] [--check]
  --check  fail if a committed notice differs from what the sources say
"""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "alpha-sdk" / "notices"


def between(text, start, end):
    a = text.index(start)
    b = text.index(end, a + len(start))
    return text[a:b].strip()


def dr_mp3(deps):
    text = (deps / "dr_mp3-src" / "dr_mp3.h").read_text(encoding="utf-8")
    return between(text, "This software is available as a choice", "*/")


def stb(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    return between(text, "This software is available under 2 licenses", "*/")


def freetype(deps):
    src = deps / "freetype-src"
    year = re.search(r"Copyright \(C\) 1996-(\d{4})", (src / "include" / "freetype" / "freetype.h").read_text(encoding="utf-8")).group(1)
    credit = (f"Portions of this software are copyright (c) {year} The FreeType Project (www.freetype.org).\n"
              "All rights reserved.\n\nFreeType is used under the FreeType License (FTL):\n\n")
    return credit + (src / "docs" / "FTL.TXT").read_text(encoding="utf-8")


def font_license():
    text = (ROOT / "src" / "platform" / "default_font_data.cpp").read_text(encoding="utf-8")
    return re.search(r'kDefaultFontLicense =\s*"([^"]*)"', text).group(1)


# name -> (title line, function of deps dir giving the license text)
NOTICES = {
    "alpha-star": ("alpha-star engine", lambda d: (ROOT / "LICENSE").read_text(encoding="utf-8")),
    "freetype": ("FreeType 2.14.3 (font rasterizer) - https://freetype.org", freetype),
    "sdl3": ("SDL 3.4.20 (Simple DirectMedia Layer) - https://libsdl.org", lambda d: (d / "sdl3-src" / "LICENSE.txt").read_text(encoding="utf-8")),
    "sdl3_image": ("SDL_image 3.4.8 - https://github.com/libsdl-org/SDL_image",
                   lambda d: (d / "sdl3_image-src" / "LICENSE.txt").read_text(encoding="utf-8")),
    "stb_image": ("stb_image 2.30 (inside SDL_image) - https://github.com/nothings/stb",
                  lambda d: stb(d / "sdl3_image-src" / "src" / "stb_image.h")),
    "box2d": ("Box2D 3.1.0 (physics) - https://box2d.org", lambda d: (d / "box2d-src" / "LICENSE").read_text(encoding="utf-8")),
    "entt": ("EnTT 3.15.0 (entity component system) - https://github.com/skypjack/entt",
             lambda d: (d / "entt-src" / "LICENSE").read_text(encoding="utf-8")),
    "quickjs-ng": ("QuickJS-ng 0.10.1 (JavaScript engine) - https://github.com/quickjs-ng/quickjs",
                   lambda d: (d / "quickjs-src" / "LICENSE").read_text(encoding="utf-8")),
    "rmlui": ("RmlUi 6.3 (game UI) - https://github.com/mikke89/RmlUi", lambda d: (d / "rmlui-src" / "LICENSE.txt").read_text(encoding="utf-8")),
    "stb_vorbis": ("stb_vorbis 1.25 (OGG decoder) - https://github.com/nothings/stb", lambda d: stb(d / "stb_vorbis-src" / "stb_vorbis.c")),
    "dr_mp3": ("dr_mp3 0.7.4 (MP3 decoder) - https://github.com/mackron/dr_libs", dr_mp3),
    "harfbuzz": ("HarfBuzz 12.3.2 (text shaping) - https://harfbuzz.github.io",
                 lambda d: (d / "harfbuzz-src" / "COPYING").read_text(encoding="utf-8")),
    "sheenbidi": ("SheenBidi 2.9.0 (Unicode bidirectional algorithm) - https://github.com/Tehreer/SheenBidi",
                  lambda d: (d / "sheenbidi-src" / "LICENSE").read_text(encoding="utf-8")),
    "monocypher": ("Monocypher 4.0.2 (game.jsbc encryption) - https://monocypher.org",
                   lambda d: (d / "monocypher-src" / "LICENCE.md").read_text(encoding="utf-8")),
    "sdl3_net": ("SDL_net 3.2.0 (alpha:net sockets) - https://github.com/libsdl-org/SDL_net",
                 lambda d: (d / "sdl3_net-src" / "LICENSE.txt").read_text(encoding="utf-8")),
    "wslay": ("wslay 1.1.1 (alpha:net WebSocket frames) - https://github.com/tatsuhiro-t/wslay",
              lambda d: (d / "wslay-src" / "COPYING").read_text(encoding="utf-8")),
    "mbedtls": ("Mbed TLS 3.6.8 (alpha:net wss://) - https://github.com/Mbed-TLS/mbedtls",
                lambda d: (d / "mbedtls-src" / "LICENSE").read_text(encoding="utf-8")),
    "builtin-font": ("Built-in 8x8 font (text with no assets.fonts)", lambda d: font_license()),
}


def render(title, body):
    return f"{title}\n{'=' * len(title)}\n\n{body.strip()}\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--deps", default=str(ROOT / "build" / "dev" / "_deps"))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    deps = pathlib.Path(args.deps)
    OUT.mkdir(parents=True, exist_ok=True)
    stale = []
    for name, (title, body) in NOTICES.items():
        text = render(title, body(deps))
        target = OUT / f"{name}.txt"
        if args.check:
            if not target.exists() or target.read_text(encoding="utf-8") != text:
                stale.append(name)
        else:
            target.write_text(text, encoding="utf-8", newline="\n")
    if stale:
        print("notices differ from the sources: " + ", ".join(stale) + " (run tools/make_notices.py)")
        sys.exit(1)
    print(f"{len(NOTICES)} notices {'match' if args.check else 'written to ' + str(OUT)}")


if __name__ == "__main__":
    main()
