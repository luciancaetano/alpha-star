#include "platform/decoders.hpp"

#include <SDL3/SDL.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#include <dr_mp3.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <vector>

#include "core/error.hpp"

namespace alpha::platform {

namespace {

[[noreturn]] void fail(const std::string& path, const std::string& why) {
  throw core::Error("cannot load audio \"" + path + "\": " + why);
}

// The whole (compressed) file: decoders read from memory, so no file stays open
// while music plays (a pack can be replaced under a running game, e.g. hot reload).
std::vector<unsigned char> file_bytes(const std::string& path) {
  std::size_t size = 0;
  void* data = SDL_LoadFile(path.c_str(), &size);
  if (!data) fail(path, SDL_GetError());
  const auto* bytes = static_cast<const unsigned char*>(data);
  std::vector<unsigned char> out(bytes, bytes + size);
  SDL_free(data);
  return out;
}

class Vorbis final : public Decoder {
 public:
  explicit Vorbis(const std::string& path) : bytes_(file_bytes(path)) {
    int error = 0;
    file_ = stb_vorbis_open_memory(bytes_.data(), static_cast<int>(bytes_.size()), &error, nullptr);
    if (!file_) fail(path, "not a readable OGG Vorbis file (stb_vorbis error " + std::to_string(error) + ")");
    info_ = stb_vorbis_get_info(file_);
  }
  ~Vorbis() override { stb_vorbis_close(file_); }
  Vorbis(const Vorbis&) = delete;
  Vorbis& operator=(const Vorbis&) = delete;
  int rate() const override { return static_cast<int>(info_.sample_rate); }
  int channels() const override { return info_.channels; }
  std::size_t read(std::span<float> out) override {
    const int n = stb_vorbis_get_samples_float_interleaved(file_, info_.channels, out.data(), static_cast<int>(out.size()));
    return static_cast<std::size_t>(n);
  }
  void rewind() override { stb_vorbis_seek_start(file_); }

 private:
  std::vector<unsigned char> bytes_;
  stb_vorbis* file_ = nullptr;
  stb_vorbis_info info_{};
};

class Mp3 final : public Decoder {
 public:
  explicit Mp3(const std::string& path) : bytes_(file_bytes(path)) {
    if (!drmp3_init_memory(&mp3_, bytes_.data(), bytes_.size(), nullptr)) fail(path, "not a readable MP3 file");
  }
  ~Mp3() override { drmp3_uninit(&mp3_); }
  Mp3(const Mp3&) = delete;
  Mp3& operator=(const Mp3&) = delete;
  int rate() const override { return static_cast<int>(mp3_.sampleRate); }
  int channels() const override { return static_cast<int>(mp3_.channels); }
  std::size_t read(std::span<float> out) override {
    const auto frames = out.size() / static_cast<std::size_t>(channels());
    return static_cast<std::size_t>(drmp3_read_pcm_frames_f32(&mp3_, frames, out.data()));
  }
  void rewind() override { drmp3_seek_to_pcm_frame(&mp3_, 0); }

 private:
  std::vector<unsigned char> bytes_;
  drmp3 mp3_{};
};

// WAV is uncompressed: decoded whole to floats at its own rate, then served in chunks.
class Wav final : public Decoder {
 public:
  explicit Wav(const std::string& path) {
    Uint8* data = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.c_str(), &spec_, &data, &length)) fail(path, SDL_GetError());
    const SDL_AudioSpec floats{SDL_AUDIO_F32, spec_.channels, spec_.freq};
    Uint8* converted = nullptr;
    int size = 0;
    const bool ok = SDL_ConvertAudioSamples(&spec_, data, static_cast<int>(length), &floats, &converted, &size);
    SDL_free(data);
    if (!ok) fail(path, SDL_GetError());
    samples_.resize(static_cast<std::size_t>(size) / sizeof(float));
    std::memcpy(samples_.data(), converted, samples_.size() * sizeof(float));
    SDL_free(converted);
  }
  int rate() const override { return spec_.freq; }
  int channels() const override { return spec_.channels; }
  std::size_t read(std::span<float> out) override {
    const auto ch = static_cast<std::size_t>(channels());
    const std::size_t n = std::min(out.size(), samples_.size() - at_) / ch * ch;
    std::copy_n(samples_.begin() + static_cast<std::ptrdiff_t>(at_), n, out.begin());
    at_ += n;
    return n / ch;
  }
  void rewind() override { at_ = 0; }

 private:
  SDL_AudioSpec spec_{};
  std::vector<float> samples_;
  std::size_t at_ = 0;
};

std::string lower_extension(const std::string& path) {
  std::string ext = std::filesystem::path(path).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

}  // namespace

std::unique_ptr<Decoder> open_decoder(const std::string& path) {
  const std::string ext = lower_extension(path);
  if (ext == ".ogg") return std::make_unique<Vorbis>(path);
  if (ext == ".mp3") return std::make_unique<Mp3>(path);
  if (ext == ".wav") return std::make_unique<Wav>(path);
  fail(path, "unsupported format \"" + ext + "\"; use .ogg (Vorbis), .mp3 or .wav");
}

}  // namespace alpha::platform
