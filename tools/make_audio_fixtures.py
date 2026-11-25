"""Generates the audio decoder fixtures in tests/fixtures/audio (deterministic input).

Encoding OGG Vorbis / MP3 needs libsndfile >= 1.1 through the `soundfile` package,
which the harness does not need otherwise, so it is installed on demand:

    python -m pip install --target <scratch dir> soundfile numpy
    PYTHONPATH=<scratch dir> python tools/make_audio_fixtures.py

Each fixture: 0.25 s at 44100 Hz, stereo; left = 440 Hz sine at 0.5, right = silence.
The tests check length, rate conversion and which side carries the tone, never
exact samples (lossy codecs).
"""
import math
import pathlib

import numpy as np
import soundfile as sf

RATE = 44100
SECONDS = 0.25
OUT = pathlib.Path(__file__).resolve().parent.parent / "tests" / "fixtures" / "audio"


def tone() -> np.ndarray:
    n = int(RATE * SECONDS)
    t = np.arange(n) / RATE
    left = 0.5 * np.sin(2 * math.pi * 440 * t)
    return np.stack([left, np.zeros(n)], axis=1).astype(np.float32)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    data = tone()
    sf.write(OUT / "tone.ogg", data, RATE, format="OGG", subtype="VORBIS")
    sf.write(OUT / "tone.mp3", data, RATE, format="MP3", subtype="MPEG_LAYER_III")
    for f in sorted(OUT.iterdir()):
        print(f"{f.name}: {f.stat().st_size} bytes")


if __name__ == "__main__":
    main()
