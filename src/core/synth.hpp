#pragma once

#include <span>
#include <vector>

namespace alpha::core {

enum class Wave { Sine, Square, Triangle, Saw, Noise };

// One procedural note. Frequency slides linearly from start to end.
struct Tone {
  Wave wave = Wave::Sine;
  float freq_start = 440.0f;  // Hz
  float freq_end = 440.0f;    // Hz
  float duration = 0.1f;      // seconds
  float volume = 0.5f;        // 0..1 peak
  float attack = 0.005f;      // seconds of linear fade-in
  float release = 0.05f;      // seconds of linear fade-out at the end
};

// Renders tones one after another as mono samples in [-1, 1].
// Deterministic: noise uses a fixed seed, so a spec always sounds the same.
std::vector<float> synthesize(std::span<const Tone> tones, int sample_rate);

}  // namespace alpha::core
