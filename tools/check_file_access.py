"""Fail if engine code touches files without the file layer (src/platform/vfs).

Usage: python tools/check_file_access.py [--root DIR] [--self-test]

Every file read/write/list in src/ goes through platform/vfs (SDL_IOStream
based: one code path for folders, Android APK assets and, later, archives;
no file stays open). This greps src/**/*.{cpp,hpp} (comments stripped) for
direct file APIs: iostreams, stdio fopen, path-taking SDL / SDL_image /
FreeType / stb_vorbis / dr_mp3 loaders, SDL filesystem calls and
std::filesystem calls that touch the disk. Only ALLOWED files may use them.

--self-test plants a `std::ifstream` in a scratch tree and fails unless the
check catches it (the lever proves it bites, every gate run).
"""
import argparse
import re
import sys
import tempfile
from pathlib import Path

import harness_config as cfg

ALLOWED = {"src/platform/vfs.cpp"}

BANNED = [
    (r"#\s*include\s*<fstream>", "<fstream>"),
    (r"\b(?:std::)?(?:i|o)?fstream\b", "std::fstream"),
    (r"\b(?:_w)?f(?:re)?open(?:_s)?\s*\(", "fopen/freopen"),
    (r"\bIMG_Load(?:Texture)?\s*\(", "IMG_Load(path)"),
    (r"\bIMG_Save(?:PNG|JPG|AVIF)\s*\(", "IMG_Save*(path)"),
    (r"\bSDL_(?:LoadWAV|LoadBMP|SaveBMP|LoadFile|SaveFile|IOFromFile)\s*\(", "SDL path-based file call"),
    (r"\bSDL_(?:GetPathInfo|GlobDirectory|EnumerateDirectory|RenamePath|RemovePath|CopyFile|CreateDirectory)\s*\(",
     "SDL filesystem call"),
    (r"\bFT_New_Face\s*\(", "FT_New_Face(path)"),
    (r"\b(?:stb_vorbis_open_filename|stb_vorbis_decode_filename|drmp3_init_file|drmp3_open_file\w*|stbi_load)\s*\(",
     "codec file API"),
    (r"\b(?:fs|filesystem)::(?:exists|is_directory|is_regular_file|file_size|last_write_time|status|"
     r"(?:recursive_)?directory_iterator|rename|remove(?:_all)?|copy(?:_file)?|create_director(?:y|ies))\b",
     "std::filesystem disk call"),
]
PATTERNS = [(re.compile(p), what) for p, what in BANNED]
COMMENTS = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)


def strip_comments(text):
    """Comments blanked, line numbers kept."""
    return COMMENTS.sub(lambda m: "\n" * m.group(0).count("\n"), text)


def violations(src, root):
    """[(relative path, line, what)] for every banned call outside ALLOWED."""
    found = []
    for path in sorted(p for p in src.rglob("*") if p.suffix in cfg.CPP_EXT):
        name = path.relative_to(root).as_posix()
        if name in ALLOWED:
            continue
        lines = strip_comments(path.read_text(encoding="utf-8")).splitlines()
        for number, line in enumerate(lines, 1):
            found += [(name, number, what) for pattern, what in PATTERNS if pattern.search(line)]
    return found


def self_test():
    """True when a planted std::ifstream is caught and the allowlisted file is not."""
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "src" / "platform").mkdir(parents=True)
        (root / "src" / "game.cpp").write_text("void f() { std::ifstream in(\"x\"); }\n", encoding="utf-8")
        (root / "src" / "platform" / "vfs.cpp").write_text("void g() { SDL_LoadFile(p, &n); }\n", encoding="utf-8")
        (root / "src" / "note.cpp").write_text("// std::ifstream is banned\n", encoding="utf-8")
        return violations(root / "src", root) == [("src/game.cpp", 1, "std::fstream")]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, default=cfg.ROOT)
    ap.add_argument("--self-test", action="store_true")
    a = ap.parse_args()
    if a.self_test and not self_test():
        print("FAIL: the file-access check did not catch a planted std::ifstream")
        return 1
    found = violations(a.root / "src", a.root)
    for name, number, what in found:
        print(f"{name}:{number}: {what} - use platform/vfs (the engine's only file layer)")
    print(f"file access: {len(found)} direct file API use(s) outside {', '.join(sorted(ALLOWED))}")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
