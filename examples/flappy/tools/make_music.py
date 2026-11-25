"""Writes assets/music/theme.ogg: a short looping chiptune for Flappy, made here
from code (no samples, no downloads), released as CC0 like the rest of the
generated assets. Deterministic: the same script always writes the same notes.

Encoding OGG Vorbis needs the `soundfile` package (libsndfile >= 1.1):

    python -m pip install --target <scratch dir> soundfile numpy
    PYTHONPATH=<scratch dir> python tools/make_music.py
"""
import pathlib

import numpy as np
import soundfile as sf

RATE = 32000
BPM = 150
STEP = 60 / BPM / 2  # eighth notes
OUT = pathlib.Path(__file__).resolve().parent.parent / "assets" / "music" / "theme.ogg"

# Lead melody (MIDI notes, None = rest), 4 bars of 8 eighths, played twice.
LEAD = [
    76, None, 79, 76, 74, 72, None, 74,
    76, 76, 79, None, 81, 79, 76, None,
    72, None, 74, 76, 79, 76, 74, 72,
    74, None, 76, 74, 72, None, 71, None,
]
# Bass roots per bar (C, A minor, F, G), pulsing eighths an octave apart.
BASS = [48, 45, 41, 43]


def freq(note: int) -> float:
    return 440.0 * 2 ** ((note - 69) / 12)


def envelope(n: int) -> np.ndarray:
    t = np.arange(n) / RATE
    return np.minimum(1.0, t / 0.005) * np.exp(-t * 6.0)


def square(note: int, n: int, duty: float = 0.5) -> np.ndarray:
    phase = (np.arange(n) * freq(note) / RATE) % 1.0
    return np.where(phase < duty, 1.0, -1.0) * envelope(n)


def triangle(note: int, n: int) -> np.ndarray:
    phase = (np.arange(n) * freq(note) / RATE) % 1.0
    return (4 * np.abs(phase - 0.5) - 1) * np.minimum(1.0, np.arange(n) / (RATE * 0.003))


def render() -> np.ndarray:
    step = int(RATE * STEP)
    lead = np.concatenate([square(n, step, 0.25) if n else np.zeros(step) for n in LEAD * 2])
    bass = np.concatenate([triangle(root + 12 * (i % 2), step) for root in BASS * 2 for i in range(8)])
    mix = 0.18 * lead + 0.30 * bass
    fade = int(RATE * 0.004)  # no click at the loop seam
    mix[:fade] *= np.linspace(0, 1, fade)
    mix[-fade:] *= np.linspace(1, 0, fade)
    return mix.astype(np.float32)


def main() -> None:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    data = render()
    sf.write(OUT, data, RATE, format="OGG", subtype="VORBIS")
    print(f"{OUT.name}: {len(data) / RATE:.2f} s, {OUT.stat().st_size} bytes")


if __name__ == "__main__":
    main()
