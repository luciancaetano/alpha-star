#include "platform/music_stream.hpp"

#include <SDL3/SDL.h>

#include <cstring>
#include <memory>
#include <utility>

#include "core/error.hpp"
#include "core/mixer.hpp"
#include "platform/decoders.hpp"

namespace alpha::platform {

namespace {

constexpr int kFloat = static_cast<int>(sizeof(float));

// A decoder behind an SDL_AudioStream that converts to stereo at the mixer rate.
class Converted final : public core::Stream {
 public:
  Converted(std::unique_ptr<Decoder> decoder, const std::string& path) : decoder_(std::move(decoder)) {
    const SDL_AudioSpec src{SDL_AUDIO_F32, decoder_->channels(), decoder_->rate()};
    const SDL_AudioSpec dst{SDL_AUDIO_F32, 2, core::Mixer::kSampleRate};
    stream_ = SDL_CreateAudioStream(&src, &dst);
    if (!stream_) throw core::Error("cannot play music \"" + path + "\": " + SDL_GetError());
    chunk_.resize(static_cast<std::size_t>(kChunkFrames * decoder_->channels()));
  }
  ~Converted() override { SDL_DestroyAudioStream(stream_); }
  Converted(const Converted&) = delete;
  Converted& operator=(const Converted&) = delete;

  std::size_t read(std::span<float> stereo) override {
    const int want = static_cast<int>(stereo.size() / 2 * 2) * kFloat;
    while (!drained_ && SDL_GetAudioStreamAvailable(stream_) < want) decode();
    const int got = SDL_GetAudioStreamData(stream_, stereo.data(), want);
    return got <= 0 ? 0 : static_cast<std::size_t>(got / (2 * kFloat));
  }

  void rewind() override {
    decoder_->rewind();
    SDL_ClearAudioStream(stream_);
    drained_ = false;
  }

 private:
  void decode() {
    const std::size_t frames = decoder_->read(chunk_);
    if (frames == 0) {
      SDL_FlushAudioStream(stream_);
      drained_ = true;
      return;
    }
    const auto floats = frames * static_cast<std::size_t>(decoder_->channels());
    SDL_PutAudioStreamData(stream_, chunk_.data(), static_cast<int>(floats) * kFloat);
  }

  std::unique_ptr<Decoder> decoder_;
  SDL_AudioStream* stream_ = nullptr;
  std::vector<float> chunk_;
  bool drained_ = false;
};

}  // namespace

core::StreamPtr open_music(const std::string& path) {
  return std::make_unique<Converted>(open_decoder(path), path);
}

std::vector<float> load_sound(const std::string& path) {
  const auto decoder = open_decoder(path);
  std::vector<float> all;
  std::vector<float> chunk(static_cast<std::size_t>(kChunkFrames * decoder->channels()));
  while (const std::size_t frames = decoder->read(chunk)) {
    all.insert(all.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(frames * static_cast<std::size_t>(decoder->channels())));
  }
  const SDL_AudioSpec src{SDL_AUDIO_F32, decoder->channels(), decoder->rate()};
  const SDL_AudioSpec mono{SDL_AUDIO_F32, 1, core::Mixer::kSampleRate};
  Uint8* converted = nullptr;
  int size = 0;
  const auto* bytes = reinterpret_cast<const Uint8*>(all.data());
  if (!SDL_ConvertAudioSamples(&src, bytes, static_cast<int>(all.size()) * kFloat, &mono, &converted, &size)) {
    throw core::Error("cannot convert sound \"" + path + "\": " + SDL_GetError());
  }
  std::vector<float> samples(static_cast<std::size_t>(size) / sizeof(float));
  std::memcpy(samples.data(), converted, samples.size() * sizeof(float));
  SDL_free(converted);
  return samples;
}

}  // namespace alpha::platform
