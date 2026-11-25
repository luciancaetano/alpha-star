#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace alpha::core {

// Decoded music read incrementally: interleaved stereo floats at
// Mixer::kSampleRate. The platform decodes files into it a little at a time.
class Stream {
 public:
  virtual ~Stream() = default;
  // Fills up to stereo.size() / 2 frames; returns how many. Fewer = the end.
  virtual std::size_t read(std::span<float> stereo) = 0;
  // Back to the first frame (looping).
  virtual void rewind() = 0;
};
using StreamPtr = std::unique_ptr<Stream>;

struct MusicOptions {
  float volume = 1.0f;
  bool loop = true;
  float fade_in = 0.0f;  // seconds
};

// One playing music stream with its fade. Gain ramps linearly toward the
// target (0 or 1) per output frame.
class MusicTrack {
 public:
  MusicTrack(std::string name, StreamPtr stream, MusicOptions options);
  const std::string& name() const { return name_; }
  // Ramps the gain to `target` over `seconds` (0 = at once).
  void fade_to(float target, float seconds);
  bool fading_out() const { return target_ == 0.0f; }
  // Faded out completely, or a non-looping stream ended.
  bool finished() const { return ended_ || (fading_out() && gain_ <= 0.0f); }
  // Adds this track into `out` (interleaved stereo), scaled by `scale`.
  void render(std::span<float> out, float scale);

 private:
  // Reads frames into `buffer`, rewinding when looping; returns frames read.
  std::size_t fill(std::span<float> buffer);

  std::string name_;
  StreamPtr stream_;
  MusicOptions options_;
  float gain_ = 1.0f;
  float target_ = 1.0f;
  float step_ = 0.0f;  // gain change per frame
  bool ended_ = false;
  std::vector<float> scratch_;
};

}  // namespace alpha::core
