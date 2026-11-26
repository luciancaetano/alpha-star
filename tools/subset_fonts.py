"""Cuts a font down to the characters a game shows, keeping every OpenType
layout feature (ligatures, Arabic joining, Indic conjuncts) so shaping still
works. Noto CJK fonts are 8+ MB; a subset of the strings a game uses is a few
KB. Needs fontTools (`pip install fonttools`); not part of the gate.

usage: python tools/subset_fonts.py SOURCE OUT (--text "..." | --text-file FILE...)
  Characters of every text file (UTF-8, e.g. the game's locale JSON) and of
  --text are kept, plus ASCII digits and space. Keep the font's OFL.txt next to it.
"""
import argparse
import pathlib
import sys


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source")
    ap.add_argument("out")
    ap.add_argument("--text", default="")
    ap.add_argument("--text-file", nargs="*", default=[])
    args = ap.parse_args()
    try:
        from fontTools import subset
    except ImportError:
        sys.exit("needs fontTools: pip install fonttools")
    text = args.text + " 0123456789"
    for f in args.text_file:
        text += pathlib.Path(f).read_text(encoding="utf-8")
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.notdef_outline = True
    options.hinting = True
    font = subset.load_font(args.source, options)
    sub = subset.Subsetter(options)
    sub.populate(text="".join(sorted(set(text))))
    sub.subset(font)
    subset.save_font(font, args.out, options)
    print(f"{args.out}: {pathlib.Path(args.out).stat().st_size} bytes")


if __name__ == "__main__":
    main()
