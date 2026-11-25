#include "core/synth.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace alpha::core {

namespace {

constexpr float kTau = 2.0f * std::numbers::pi_v<float>;

// phase in [0, 1); noise ignores it.
float wave_sample(Wave wave, float phase, std::uint32_t& noise) {
  switch (wave) {
    case Wave::Sine: return std::sin(kTau * phase);
    case Wave::Square: return phase < 0.5f ? 1.0f : -1.0f;
    case Wave::Triangle: return 1.0f - 4.0f * std::abs(phase - 0.5f);
    case Wave::Saw: return 2.0f * phase - 1.0f;
    case Wave::Noise: break;
  }
  noise = noise * 1664525u + 1013904223u;
  return static_cast<float>(noise >> 8) / 8388608.0f - 1.0f;
}

float envelope(const Tone& t, float time) {
  float gain = 1.0f;
  if (t.attack > 0.0f) gain = std::min(gain, time / t.attack);
  if (t.release > 0.0f) gain = std::min(gain, (t.duration - time) / t.release);
  return std::clamp(gain, 0.0f, 1.0f);
}

void render(const Tone& t, int sample_rate, std::vector<float>& out) {
  const auto count = static_cast<std::size_t>(t.duration * static_cast<float>(sample_rate) + 0.5f);
  const float dt = 1.0f / static_cast<float>(sample_rate);
  std::uint32_t noise = 0x2545F491u;
  float phase = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const float time = static_cast<float>(i) * dt;
    const float progress = t.duration > 0.0f ? time / t.duration : 0.0f;
    const float freq = t.freq_start + (t.freq_end - t.freq_start) * progress;
    out.push_back(wave_sample(t.wave, phase, noise) * t.volume * envelope(t, time + dt));
    phase += freq * dt;
    phase -= std::floor(phase);
  }
}

}  // namespace

std::vector<float> synthesize(std::span<const Tone> tones, int sample_rate) {
  std::vector<float> out;
  for (const Tone& t : tones) render(t, sample_rate, out);
  return out;
}

}  // namespace alpha::core
