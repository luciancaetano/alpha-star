#include "core/music_track.hpp"

#include <algorithm>
#include <utility>

#include "core/mixer.hpp"

namespace alpha::core {

MusicTrack::MusicTrack(std::string name, StreamPtr stream, MusicOptions options)
    : name_(std::move(name)), stream_(std::move(stream)), options_(options) {
  if (options.fade_in > 0.0f) {
    gain_ = 0.0f;
    fade_to(1.0f, options.fade_in);
  }
}

void MusicTrack::fade_to(float target, float seconds) {
  target_ = target;
  const float frames = seconds * static_cast<float>(Mixer::kSampleRate);
  if (frames < 1.0f) {
    gain_ = target;
    step_ = 0.0f;
    return;
  }
  step_ = (target - gain_) / frames;
}

std::size_t MusicTrack::fill(std::span<float> buffer) {
  std::size_t frames = 0;
  bool rewound = false;  // twice in a row without a frame: an empty stream
  while (frames * 2 < buffer.size()) {
    const std::size_t got = stream_->read(buffer.subspan(frames * 2));
    frames += got;
    if (got > 0) {
      rewound = false;
      continue;
    }
    if (!options_.loop || rewound) break;
    stream_->rewind();
    rewound = true;
  }
  return frames;
}

void MusicTrack::render(std::span<float> out, float scale) {
  scratch_.resize(out.size());
  const std::size_t frames = fill(scratch_);
  ended_ = frames * 2 < out.size();
  for (std::size_t f = 0; f < frames; ++f) {
    const float g = gain_ * options_.volume * scale;
    out[f * 2] += scratch_[f * 2] * g;
    out[f * 2 + 1] += scratch_[f * 2 + 1] * g;
    gain_ = std::clamp(gain_ + step_, 0.0f, 1.0f);
  }
}

}  // namespace alpha::core
