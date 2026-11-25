#include <doctest/doctest.h>

#include <memory>
#include <vector>

#include "core/mixer.hpp"

using namespace alpha::core;

namespace {

SoundPtr constant(float value, std::size_t count) {
  return std::make_shared<const std::vector<float>>(count, value);
}

PlayOptions opts(float volume, float pitch, bool loop) {
  PlayOptions o;
  o.volume = volume;
  o.pitch = pitch;
  o.loop = loop;
  return o;
}

// A stream of `frames` stereo frames, every sample = value; counts rewinds.
class Flat final : public Stream {
 public:
  Flat(float value, std::size_t frames, int* rewinds = nullptr) : value_(value), frames_(frames), rewinds_(rewinds) {}
  std::size_t read(std::span<float> stereo) override {
    const std::size_t n = std::min(stereo.size() / 2, frames_ - at_);
    for (std::size_t i = 0; i < n * 2; ++i) stereo[i] = value_;
    at_ += n;
    return n;
  }
  void rewind() override {
    at_ = 0;
    if (rewinds_) ++*rewinds_;
  }

 private:
  float value_;
  std::size_t frames_;
  std::size_t at_ = 0;
  int* rewinds_;
};

MusicOptions music_opts(float volume, bool loop, float fade_in) {
  MusicOptions o;
  o.volume = volume;
  o.loop = loop;
  o.fade_in = fade_in;
  return o;
}

}  // namespace

TEST_CASE("mixer output is interleaved stereo; mono sounds go to both sides") {
  Mixer m;
  m.play(constant(0.25f, 4), {});
  m.play(constant(0.25f, 4), opts(0.5f, 1.0f, false));
  std::vector<float> out(8);
  m.mix(out);
  CHECK(out[0] == doctest::Approx(0.375f));
  CHECK(out[1] == doctest::Approx(0.375f));
  CHECK(out[7] == doctest::Approx(0.375f));
}

TEST_CASE("pan moves a voice between left and right; center keeps full volume") {
  Mixer m;
  PlayOptions left = opts(1.0f, 1.0f, false);
  left.pan = -1.0f;
  m.play(constant(0.5f, 2), left);
  std::vector<float> out(4);
  m.mix(out);
  CHECK(out[0] == doctest::Approx(0.5f));
  CHECK(out[1] == doctest::Approx(0.0f));
  PlayOptions half = opts(1.0f, 1.0f, false);
  half.pan = 0.5f;
  m.play(constant(0.5f, 2), half);
  m.mix(out);
  CHECK(out[0] == doctest::Approx(0.25f));
  CHECK(out[1] == doctest::Approx(0.5f));
}

TEST_CASE("buses scale their voices; master scales everything; output clamps") {
  Mixer m;
  CHECK(m.bus_names() == std::vector<std::string>{"master", "music", "sfx", "voice"});
  const BusId ambience = m.add_bus("ambience");
  CHECK(m.add_bus("ambience") == ambience);
  CHECK(m.find_bus("ambience") == ambience);
  CHECK_FALSE(m.find_bus("nope").has_value());
  PlayOptions a = opts(1.0f, 1.0f, false);
  a.bus = ambience;
  m.play(constant(0.5f, 2), a);
  PlayOptions v = opts(1.0f, 1.0f, false);
  v.bus = Mixer::kVoice;
  m.play(constant(0.5f, 2), v);
  m.set_bus_volume(ambience, 0.5f);
  m.set_bus_volume(Mixer::kMaster, 0.5f);
  CHECK(m.bus_volume(ambience) == 0.5f);
  std::vector<float> out(4);
  m.mix(out);
  CHECK(out[0] == doctest::Approx((0.25f + 0.5f) * 0.5f));
  m.set_bus_volume(Mixer::kMaster, 1.0f);
  m.play(constant(1.0f, 2), {});
  m.play(constant(1.0f, 2), {});
  m.mix(out);
  CHECK(out[0] == doctest::Approx(1.0f));
}

TEST_CASE("mixer drops finished voices, loops, pitches and stops") {
  Mixer m;
  const VoiceId once = m.play(constant(1.0f, 3), {});
  const VoiceId loop = m.play(constant(1.0f, 2), opts(1.0f, 1.0f, true));
  const VoiceId fast = m.play(constant(1.0f, 4), opts(1.0f, 2.0f, false));
  std::vector<float> out(6);
  m.mix(out);
  CHECK_FALSE(m.playing(fast));  // 4 samples at 2x pitch = 2 output frames
  CHECK(out[4] == doctest::Approx(1.0f));
  m.mix(out);
  CHECK_FALSE(m.playing(once));
  CHECK(m.playing(loop));
  m.stop(loop);
  CHECK(m.voices() == 0);
  m.play(constant(1.0f, 9), {});
  m.stop_all();
  m.mix(out);
  CHECK(out[0] == 0.0f);
}

TEST_CASE("music streams through the music bus and loops by rewinding") {
  Mixer m;
  int rewinds = 0;
  m.music("theme", std::make_unique<Flat>(0.5f, 3, &rewinds), music_opts(1.0f, true, 0.0f));
  CHECK(m.music_name() == "theme");
  m.set_bus_volume(Mixer::kMusic, 0.5f);
  std::vector<float> out(10);  // 5 frames from a 3-frame stream
  m.mix(out);
  CHECK(rewinds == 1);
  CHECK(out[9] == doctest::Approx(0.25f));
  m.music("once", std::make_unique<Flat>(0.5f, 2), music_opts(1.0f, false, 0.0f));
  m.mix(out);
  CHECK(out[2] == doctest::Approx(0.25f));
  CHECK(out[4] == 0.0f);
  CHECK(m.music_name().empty());
}

TEST_CASE("music fades in, crossfades and fades out over seconds") {
  Mixer m;
  constexpr std::size_t kSecond = Mixer::kSampleRate;
  m.music("a", std::make_unique<Flat>(1.0f, kSecond * 4), music_opts(1.0f, true, 1.0f));
  std::vector<float> half(kSecond);  // 0.5 s of stereo
  m.mix(half);
  CHECK(half[0] < 0.01f);
  CHECK(half[kSecond - 1] == doctest::Approx(0.5f).epsilon(0.01));
  m.mix(half);
  m.crossfade("b", std::make_unique<Flat>(0.5f, kSecond * 4), music_opts(1.0f, true, 0.0f), 1.0f);
  CHECK(m.music_name() == "b");
  m.mix(half);  // a: 1 -> 0.5, b: 0 -> 0.5 * 0.5
  CHECK(half[kSecond - 1] == doctest::Approx(0.5f + 0.25f).epsilon(0.01));
  m.mix(half);
  m.mix(half);
  CHECK(half[0] == doctest::Approx(0.5f).epsilon(0.01));  // only b now
  m.stop_music(0.5f);
  CHECK(m.music_name().empty());
  m.mix(half);
  CHECK(half[kSecond - 1] < 0.01f);
  m.music("c", std::make_unique<Flat>(1.0f, 8), music_opts(0.5f, true, 0.0f));
  m.music("d", std::make_unique<Flat>(1.0f, 8), music_opts(1.0f, true, 0.0f));  // replaces c at once
  std::vector<float> out(2);
  m.mix(out);
  CHECK(out[0] == doctest::Approx(1.0f));
}
