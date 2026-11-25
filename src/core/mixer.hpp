#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace alpha::core {

// Mono float samples at Mixer::kSampleRate. Shared: voices keep it alive.
using SoundPtr = std::shared_ptr<const std::vector<float>>;
using VoiceId = std::uint32_t;

struct PlayOptions {
  float volume = 1.0f;
  float pitch = 1.0f;  // playback speed; 2 = one octave up
  bool loop = false;
};

// Software mixer. Single-threaded by design: the main loop calls mix() and
// hands the result to the audio device, so no state is shared with another thread.
class Mixer {
 public:
  static constexpr int kSampleRate = 48000;

  VoiceId play(SoundPtr sound, PlayOptions options);
  void stop(VoiceId id);
  void stop_all();
  bool playing(VoiceId id) const;
  std::size_t voices() const { return voices_.size(); }

  void set_volume(float master) { master_ = master; }
  float volume() const { return master_; }

  // Overwrites `out` with the next samples and advances every voice.
  void mix(std::span<float> out);

 private:
  struct Voice {
    VoiceId id;
    SoundPtr sound;
    PlayOptions options;
    double position = 0.0;
  };
  // Adds one voice into `out`; returns false when it finished.
  bool render(Voice& voice, std::span<float> out) const;

  std::vector<Voice> voices_;
  VoiceId next_id_ = 1;
  float master_ = 1.0f;
};

}  // namespace alpha::core
