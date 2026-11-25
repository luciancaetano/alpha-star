"""Render the CC0 orbital synth score: original notes, no samples/downloads.

Deterministic 16-bar stereo loop in E minor, 104 BPM: glass arpeggios,
slow luminous pads, warm bass and a quiet electronic heartbeat.
Requires numpy and soundfile (libsndfile >= 1.1).

    python tools/make_music.py
"""
import pathlib

import numpy as np
import soundfile as sf

RATE = 32000
BPM = 104
STEP = round(RATE * 60 / BPM / 2)
LENGTH = STEP * 128
OUT = pathlib.Path(__file__).resolve().parent.parent / "assets/music/theme.ogg"
CHORDS = [(52, 55, 59, 66), (48, 52, 55, 59), (43, 50, 54, 59), (50, 57, 59, 64)]
ARP = [0, 2, 1, 3, 2, 1, 3, 2]


def freq(note: int) -> float:
    return 440.0 * 2 ** ((note - 69) / 12)


def envelope(t: np.ndarray, attack: float, release: float) -> np.ndarray:
    return np.minimum(t / attack, 1) * np.minimum((len(t) / RATE - t) / release, 1)


def glass(note: int) -> np.ndarray:
    t = np.arange(STEP * 4) / RATE
    phase = 2 * np.pi * freq(note) * t
    # A decaying FM overtone makes a soft, bell-like sci-fi pluck.
    wave = np.sin(phase + 0.65 * np.sin(phase * 2) * np.exp(-t * 8))
    return wave * np.exp(-t * 4.5) * envelope(t, 0.012, 0.12)


def pad(chord: tuple[int, ...]) -> np.ndarray:
    t = np.arange(STEP * 20) / RATE
    wave = np.zeros((len(t), 2))
    for note in chord:
        phase = 2 * np.pi * freq(note) * t
        wave[:, 0] += np.sin(phase * 0.999) + 0.2 * np.sin(phase * 2)
        wave[:, 1] += np.sin(phase * 1.001) + 0.2 * np.sin(phase * 2)
    return wave * envelope(t, 0.65, 1.5)[:, None] / len(chord)


def bass(note: int) -> np.ndarray:
    t = np.arange(STEP * 2) / RATE
    phase = 2 * np.pi * freq(note) * t
    wave = np.sin(phase) + 0.17 * np.sin(phase * 2)
    return wave * envelope(t, 0.02, 0.16) * np.exp(-t * 1.8)


def heartbeat() -> np.ndarray:
    t = np.arange(STEP) / RATE
    phase = 2 * np.pi * (48 * t + 35 * (1 - np.exp(-t * 25)) / 25)
    return np.sin(phase) * np.exp(-t * 20) * envelope(t, 0.005, 0.05)


def place(mix: np.ndarray, signal: np.ndarray, start: int) -> None:
    # Wrap release tails into the start so the musical loop keeps its ambience.
    indices = (np.arange(len(signal)) + start) % len(mix)
    mix[indices] += signal


def pan(signal: np.ndarray, position: float) -> np.ndarray:
    angle = (position + 1) * np.pi / 4
    return signal[:, None] * np.array([np.cos(angle), np.sin(angle)])


def arrangement() -> tuple[np.ndarray, np.ndarray]:
    mix = np.zeros((LENGTH, 2))
    arps = np.zeros_like(mix)
    for section in range(8):
        chord = CHORDS[section % len(CHORDS)]
        start = section * 16 * STEP
        place(mix, pad(chord) * 0.12, start)
        for eighth in range(16):
            at = start + eighth * STEP
            note = chord[ARP[eighth % 8]] + 12
            position = 0.45 * np.sin(eighth * np.pi / 4)
            place(arps, pan(glass(note), position) * 0.13, at)
            if eighth % 2 == 0:
                place(mix, pan(bass(chord[0] - 12), 0) * 0.16, at)
            if eighth % 4 == 0:
                place(mix, pan(heartbeat(), 0) * 0.09, at)
    return mix, arps


def render() -> np.ndarray:
    mix, arps = arrangement()
    mix += arps
    for delay, gain in [(3, 0.30), (6, 0.13), (9, 0.06)]:
        mix += np.roll(arps[:, ::-1], delay * STEP, axis=0) * gain
    mix -= mix.mean(axis=0)
    mix *= 0.70 / np.max(np.abs(mix))
    # A tiny edge ramp also avoids a click when playback starts/stops mid-loop.
    edge = round(RATE * 0.004)
    mix[:edge] *= np.linspace(0, 1, edge)[:, None]
    mix[-edge:] *= np.linspace(1, 0, edge)[:, None]
    return mix.astype(np.float32)


def main() -> None:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    data = render()
    # Keep Vorbis writes bounded; a whole-song write can exhaust its encoder.
    with sf.SoundFile(OUT, "w", samplerate=RATE, channels=2, format="OGG", subtype="VORBIS") as output:
        for start in range(0, len(data), 4096):
            output.write(data[start:start + 4096])
    print(f"{OUT.name}: {len(data) / RATE:.2f} s, stereo, {BPM} BPM, {OUT.stat().st_size} bytes")


if __name__ == "__main__":
    main()
