#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "core/mixer.hpp"

namespace alpha::platform {

// Default playback device (stereo) fed from the main thread (push model, no
// callback); streamed music decodes inside pump, on the main thread too.
// If no device opens, the game keeps running silently.
class AudioOut {
 public:
  AudioOut();
  ~AudioOut();
  AudioOut(const AudioOut&) = delete;
  AudioOut& operator=(const AudioOut&) = delete;

  bool ok() const { return stream_ != nullptr; }
  // Tops the device queue up to ~kLatency frames from the mixer. Call every frame.
  void pump(core::Mixer& mixer);
  // The app paused: the device stops and pump() mixes nothing (music keeps
  // its place); false resumes. Repeating the current state does nothing.
  void pause(bool paused);
  bool paused() const { return paused_; }
  // Stereo frames queued in the device and not yet played.
  int queued() const;
  static constexpr int kFrameBytes = 2 * static_cast<int>(sizeof(float));

  // Queue target: ~2 frames at 60 Hz. Lower risks gaps when a frame runs late;
  // higher makes sounds lag behind the action.
  static constexpr int kLatency = core::Mixer::kSampleRate / 30;  // ~33 ms
  // Device buffer in sample frames (SDL's default can be several times larger).
  static constexpr const char* kDeviceFrames = "512";  // ~11 ms at 48 kHz

 private:
  SDL_AudioStream* stream_ = nullptr;
  std::vector<float> buffer_;
  bool paused_ = false;
};

}  // namespace alpha::platform
