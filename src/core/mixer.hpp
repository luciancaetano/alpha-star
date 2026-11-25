#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/music_track.hpp"

namespace alpha::core {

// Mono float samples at Mixer::kSampleRate. Shared: voices keep it alive.
using SoundPtr = std::shared_ptr<const std::vector<float>>;
using VoiceId = std::uint32_t;
// Index of a volume bus (see Mixer::bus_names).
using BusId = std::size_t;

struct PlayOptions {
  float volume = 1.0f;
  float pitch = 1.0f;  // playback speed; 2 = one octave up
  bool loop = false;
  float pan = 0.0f;    // -1 left .. 1 right; 0 = both sides at full volume
  BusId bus = 2;       // Mixer::kSfx
};

// Software stereo mixer: sound voices (mono, panned) plus streamed music
// tracks, each through a volume bus, all through the master bus.
// Single-threaded by design: the main loop calls mix() and hands the result
// to the audio device, so no state is shared with another thread.
class Mixer {
 public:
  static constexpr int kSampleRate = 48000;
  static constexpr BusId kMaster = 0;
  static constexpr BusId kMusic = 1;
  static constexpr BusId kSfx = 2;
  static constexpr BusId kVoice = 3;

  Mixer();

  VoiceId play(SoundPtr sound, PlayOptions options);
  void stop(VoiceId id);
  // Stops every sound and the music.
  void stop_all();
  bool playing(VoiceId id) const;
  std::size_t voices() const { return voices_.size(); }

  // The id of bus `name`, added at volume 1 if new.
  BusId add_bus(const std::string& name);
  std::optional<BusId> find_bus(std::string_view name) const;
  std::vector<std::string> bus_names() const;
  void set_bus_volume(BusId bus, double volume) { buses_.at(bus).second = volume; }
  double bus_volume(BusId bus) const { return buses_.at(bus).second; }

  // Starts music `name`; whatever played stops at once.
  void music(std::string name, StreamPtr stream, MusicOptions options);
  // Starts music `name` fading in over `seconds` while the current fades out.
  void crossfade(std::string name, StreamPtr stream, MusicOptions options, float seconds);
  void stop_music(float fade_out);
  // The music playing (not fading out); "" when none.
  std::string music_name() const;

  // Overwrites `out` (interleaved stereo) with the next frames and advances.
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
  std::vector<MusicTrack> tracks_;  // the last one not fading out is "the" music
  std::vector<std::pair<std::string, double>> buses_;  // double: scripts read back what they set
  VoiceId next_id_ = 1;
};

}  // namespace alpha::core
