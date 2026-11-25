#include "platform/audio_out.hpp"

namespace alpha::platform {

AudioOut::AudioOut() {
  const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, core::Mixer::kSampleRate};
  SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, kDeviceFrames);
  stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (stream_) SDL_ResumeAudioStreamDevice(stream_);
}

AudioOut::~AudioOut() { SDL_DestroyAudioStream(stream_); }

int AudioOut::queued() const {
  return ok() ? SDL_GetAudioStreamQueued(stream_) / static_cast<int>(sizeof(float)) : 0;
}

void AudioOut::pump(core::Mixer& mixer) {
  const int missing = kLatency - queued();
  if (!ok() || missing <= 0) return;
  buffer_.resize(static_cast<std::size_t>(missing));
  mixer.mix(buffer_);
  SDL_PutAudioStreamData(stream_, buffer_.data(), missing * static_cast<int>(sizeof(float)));
}

}  // namespace alpha::platform
