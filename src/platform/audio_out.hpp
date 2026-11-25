#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "core/mixer.hpp"

namespace alpha::platform {

// Default playback device fed from the main thread (push model, no callback).
// If no device opens, the game keeps running silently.
class AudioOut {
 public:
  AudioOut();
  ~AudioOut();
  AudioOut(const AudioOut&) = delete;
  AudioOut& operator=(const AudioOut&) = delete;

  bool ok() const { return stream_ != nullptr; }
  // Tops the device queue up to ~kLatency samples from the mixer. Call every frame.
  void pump(core::Mixer& mixer);
  // Samples queued in the device and not yet played.
  int queued() const;

  static constexpr int kLatency = core::Mixer::kSampleRate / 20;  // 50 ms

 private:
  SDL_AudioStream* stream_ = nullptr;
  std::vector<float> buffer_;
};

}  // namespace alpha::platform
