// The night's music: two synthesized layers started together and looped (same length, 24 s).
// A minor, 80 bpm, 8 bars: Am F C G | Am F C E. A soft melody with rests over a sine bass.
import type { Note } from "alpha";

const BEAT = 0.75;  // a quarter note at 80 bpm
const HZ: Record<string, number> = {
  E2: 82.41, F2: 87.31, G2: 98.0, A2: 110.0, B2: 123.47, C3: 130.81, D3: 146.83, E3: 164.81,
  G4: 392.0, A4: 440.0, B4: 493.88, C5: 523.25, D5: 587.33, E5: 659.25, F5: 698.46, G5: 783.99, A5: 880.0,
};

/** "E5:1" = E5 for one beat; "-:2" = two beats of rest. */
function line(score: string, wave: Note["wave"], volume: number, attack: number, release: number): Note[] {
  return score.trim().split(/\s+/).map((token) => {
    const [name, beats] = token.split(":");
    const duration = Number(beats) * BEAT;
    if (name === "-") return { wave, freq: 440, duration, volume: 0 };
    return { wave, freq: HZ[name], duration, volume, attack, release };
  });
}

// Each bar is 4 beats.
const MELODY = `
  E5:1 C5:0.5 A4:0.5 B4:1 C5:1
  A4:1.5 C5:0.5 F5:1 E5:1
  E5:1 G5:0.5 E5:0.5 D5:1 C5:1
  D5:2 -:1 B4:1
  C5:1 E5:1 A5:1 G5:0.5 E5:0.5
  F5:1.5 E5:0.5 D5:1 C5:1
  E5:0.5 D5:0.5 C5:1 G4:1 C5:1
  B4:2 -:2
`;

const BASS = `
  A2:2 E3:2   F2:2 C3:2   C3:2 G2:2   G2:2 D3:2
  A2:2 E3:2   F2:2 C3:2   C3:2 G2:2   E2:2 B2:2
`;

export const themeMelody = line(MELODY, "triangle", 0.16, 0.03, 0.35);
export const themeBass = line(BASS, "sine", 0.3, 0.08, 0.9);
