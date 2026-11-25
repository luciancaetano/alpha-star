#include <doctest/doctest.h>

#include <cmath>
#include <memory>
#include <vector>

#include "core/error.hpp"
#include "core/mixer.hpp"
#include "core/synth.hpp"

using namespace alpha::core;

namespace {

constexpr int kRate = Mixer::kSampleRate;

int zero_crossings(const std::vector<float>& s, std::size_t from, std::size_t to) {
  int n = 0;
  for (std::size_t i = from + 1; i < to; ++i) n += (s[i - 1] < 0) != (s[i] < 0);
  return n;
}

Tone tone(Wave wave, float freq, float duration) {
  Tone t;
  t.wave = wave;
  t.freq_start = freq;
  t.freq_end = freq;
  t.duration = duration;
  t.volume = 0.5f;
  t.attack = 0.0f;
  t.release = 0.0f;
  return t;
}

}  // namespace

TEST_CASE("synthesize has the requested length and frequency") {
  const std::vector<Tone> tones{tone(Wave::Sine, 440, 0.5f)};
  const auto s = synthesize(tones, kRate);
  REQUIRE(s.size() == static_cast<std::size_t>(kRate / 2));
  // 440 Hz over 0.5 s = 220 cycles = ~440 zero crossings.
  CHECK(zero_crossings(s, 0, s.size()) == doctest::Approx(440).epsilon(0.01));
}

TEST_CASE("each wave stays within volume; square sits at +-volume") {
  for (const Wave w : {Wave::Sine, Wave::Square, Wave::Triangle, Wave::Saw, Wave::Noise}) {
    const std::vector<Tone> tones{tone(w, 220, 0.05f)};
    for (const float v : synthesize(tones, kRate)) CHECK(std::abs(v) <= 0.5f + 1e-5f);
  }
  const std::vector<Tone> sq{tone(Wave::Square, 100, 0.05f)};
  for (const float v : synthesize(sq, kRate)) CHECK(std::abs(v) == doctest::Approx(0.5f));
}

TEST_CASE("frequency slides from start to end") {
  Tone t = tone(Wave::Sine, 200, 1.0f);
  t.freq_end = 800;
  const std::vector<Tone> tones{t};
  const auto s = synthesize(tones, kRate);
  const int head = zero_crossings(s, 0, kRate / 10);
  const int tail = zero_crossings(s, kRate - kRate / 10, kRate);
  CHECK(tail > head * 3);
}

TEST_CASE("envelope starts and ends silent; sequences concatenate") {
  Tone t = tone(Wave::Square, 300, 0.2f);
  t.attack = 0.05f;
  t.release = 0.05f;
  const std::vector<Tone> tones{t, tone(Wave::Sine, 300, 0.1f)};
  const auto s = synthesize(tones, kRate);
  CHECK(s.size() == static_cast<std::size_t>(kRate * 0.3f + 0.5f));
  CHECK(std::abs(s[0]) < 0.01f);
  CHECK(std::abs(s[static_cast<std::size_t>(kRate * 0.2f) - 1]) < 0.01f);
  CHECK(std::abs(s[static_cast<std::size_t>(kRate * 0.1f)]) == doctest::Approx(0.5f));
}

TEST_CASE("noise is the same every time") {
  const std::vector<Tone> tones{tone(Wave::Noise, 1, 0.01f)};
  CHECK(synthesize(tones, kRate) == synthesize(tones, kRate));
}
