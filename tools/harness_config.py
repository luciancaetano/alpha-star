"""Single source of truth for harness limits and paths."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
TESTS = ROOT / "tests"
CODE_DIRS = [SRC, TESTS]
CPP_EXT = {".cpp", ".hpp"}
UNPAIRED_CPP = {"main.cpp"}  # .cpp files allowed without a .hpp

MAX_FUNCTION_LINES = 40
MAX_FILE_LINES = 300
MAX_CCN = 10
MAX_PARAMS = 5
MAX_CRAP = 10.0
DUP_THRESHOLD = 0.8
DUP_MIN_TOKENS = 50


def cpp_files(dirs=None):
    for d in dirs or CODE_DIRS:
        yield from (p for p in sorted(d.rglob("*")) if p.suffix in CPP_EXT)


def rel(path):
    return Path(path).resolve().relative_to(ROOT).as_posix()
