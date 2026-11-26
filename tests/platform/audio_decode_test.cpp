#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/mixer.hpp"
#include "platform/music_stream.hpp"

using namespace alpha;
namespace fs = std::filesystem;

namespace {

template <typename T>
void put(std::ofstream& f, T v) {
  f.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

// 16-bit PCM WAV with `frames` frames of `channels` channels, all = value.
fs::path write_wav(const char* name, int rate, std::uint16_t channels, std::int16_t value, int frames) {
  const fs::path path = fs::temp_directory_path() / name;
  std::ofstream f(path, std::ios::binary);
  const auto data = static_cast<std::uint32_t>(frames * channels * 2);
  f.write("RIFF", 4);
  put<std::uint32_t>(f, 36 + data);
  f.write("WAVEfmt ", 8);
  put<std::uint32_t>(f, 16);
  put<std::uint16_t>(f, 1);
  put<std::uint16_t>(f, channels);
  put<std::uint32_t>(f, static_cast<std::uint32_t>(rate));
  put<std::uint32_t>(f, static_cast<std::uint32_t>(rate * channels * 2));
  put<std::uint16_t>(f, static_cast<std::uint16_t>(channels * 2));
  put<std::uint16_t>(f, 16);
  f.write("data", 4);
  put<std::uint32_t>(f, data);
  for (int i = 0; i < frames * channels; ++i) put<std::int16_t>(f, value);
  return path;
}

std::string fixture(const char* name) { return std::string(ALPHA_TEST_FIXTURES) + "/audio/" + name; }

float peak(const std::vector<float>& s, std::size_t from, std::size_t step) {
  float p = 0.0f;
  for (std::size_t i = from; i < s.size(); i += step) p = std::max(p, std::abs(s[i]));
  return p;
}

// Every stereo frame of a stream until it ends (or `limit` frames).
std::vector<float> drain(core::Stream& stream, std::size_t limit) {
  std::vector<float> all;
  std::vector<float> chunk(800 * 2);
  while (all.size() / 2 < limit) {
    const std::size_t n = stream.read(chunk);
    if (n == 0) break;
    all.insert(all.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(n * 2));
  }
  return all;
}

constexpr double kFixtureSeconds = 0.25;  // tools/make_audio_fixtures.py

}  // namespace

TEST_CASE("load_sound resamples WAV to mono 48 kHz floats") {
  const fs::path path = write_wav("alpha_test.wav", 24000, 2, 16384, 2400);
  const auto samples = platform::load_sound(path.string());
  CHECK(samples.size() == doctest::Approx(4800).epsilon(0.01));
  CHECK(samples[samples.size() / 2] == doctest::Approx(0.5f).epsilon(0.01));
  fs::remove(path);
}

TEST_CASE("load_sound decodes OGG Vorbis and MP3 to mono 48 kHz") {
  for (const char* name : {"tone.ogg", "tone.mp3"}) {
    CAPTURE(name);
    const auto samples = platform::load_sound(fixture(name));
    // MP3 adds encoder padding: allow a few frames of slack.
    CHECK(static_cast<double>(samples.size()) == doctest::Approx(kFixtureSeconds * core::Mixer::kSampleRate).epsilon(0.08));
    CHECK(peak(samples, 0, 1) == doctest::Approx(0.25f).epsilon(0.15));  // (0.5 left + 0 right) / 2
  }
}

TEST_CASE("audio files fail with the file and the supported formats") {
  CHECK_THROWS_WITH_AS(platform::load_sound("no/such.ogg"), doctest::Contains("no/such.ogg"), core::Error);
  CHECK_THROWS_WITH_AS(platform::load_sound("no/such.mp3"), doctest::Contains("cannot load audio"), core::Error);
  const fs::path text = fs::temp_directory_path() / "alpha_not_audio.mp3";
  std::ofstream(text) << "hello";
  CHECK_THROWS_WITH_AS(platform::open_music(text.string()), doctest::Contains("not a readable MP3 file"), core::Error);
  fs::remove(text);
  CHECK_THROWS_WITH_AS(platform::open_music("song.flac"), doctest::Contains(".ogg (Vorbis), .mp3 or .wav"), core::Error);
}

TEST_CASE("open_music streams stereo at 48 kHz, keeps the sides, and rewinds") {
  for (const char* name : {"tone.ogg", "tone.mp3"}) {
    CAPTURE(name);
    const auto stream = platform::open_music(fixture(name));
    const auto all = drain(*stream, 48000);
    CHECK(static_cast<double>(all.size() / 2) == doctest::Approx(kFixtureSeconds * core::Mixer::kSampleRate).epsilon(0.08));
    CHECK(peak(all, 0, 2) == doctest::Approx(0.5f).epsilon(0.15));
    CHECK(peak(all, 1, 2) < 0.05f);
    stream->rewind();
    CHECK(drain(*stream, 48000).size() == all.size());
  }
}

TEST_CASE("playing music keeps no file open: the pack can be replaced (hot reload)") {
  for (const char* name : {"tone.ogg", "tone.mp3"}) {
    CAPTURE(name);
    const fs::path copy = fs::temp_directory_path() / (std::string("alpha_open_") + name);
    fs::copy_file(fixture(name), copy, fs::copy_options::overwrite_existing);
    const auto stream = platform::open_music(copy.string());
    CHECK(fs::remove(copy));
    CHECK_FALSE(drain(*stream, 4800).empty());
  }
}

TEST_CASE("a looping streamed WAV plays through the mixer's music bus") {
  const fs::path path = write_wav("alpha_music.wav", 22050, 1, 16384, 100);
  core::Mixer m;
  core::MusicOptions o;
  m.music("loop", platform::open_music(path.string()), o);
  std::vector<float> out(48000 * 2);  // 1 s: the 100-frame file loops many times
  m.mix(out);
  CHECK(m.music_name() == "loop");
  CHECK(out[out.size() - 1] == doctest::Approx(0.5f).epsilon(0.05));
  fs::remove(path);
}

TEST_CASE("looping streamed OGG/MP3 music never ends, mixed in small per-frame chunks") {
  for (const char* name : {"tone.ogg", "tone.mp3"}) {
    CAPTURE(name);
    core::Mixer m;
    m.music("tone", platform::open_music(fixture(name)), core::MusicOptions{});
    std::vector<float> out;
    for (int i = 0; i < 400; ++i) {  // ~10 loops of the 0.25 s fixture, uneven device top-ups
      out.assign(static_cast<std::size_t>(37 + (i * 211) % 1600) * 2, 0.0f);
      m.mix(out);
      REQUIRE(m.music_name() == "tone");
    }
    CHECK(peak(out, 0, 2) > 0.1f);
  }
}

TEST_CASE("streaming cost per 60 Hz frame" * doctest::skip()) {
  // Bench: decode + convert 800 stereo frames (one frame at 60 Hz) from OGG and MP3.
  for (const char* name : {"tone.ogg", "tone.mp3"}) {
    core::Mixer m;
    m.music("bench", platform::open_music(fixture(name)), core::MusicOptions{});
    std::vector<float> out(800 * 2);
    const auto start = std::chrono::steady_clock::now();
    constexpr int kFrames = 3600;  // one minute at 60 Hz
    for (int i = 0; i < kFrames; ++i) m.mix(out);
    const std::chrono::duration<double, std::micro> took = std::chrono::steady_clock::now() - start;
    const std::string line = std::string(name) + ": " + std::to_string(took.count() / kFrames) + " us per frame";
    MESSAGE(line);
  }
}
