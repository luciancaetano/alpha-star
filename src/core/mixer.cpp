#include "core/mixer.hpp"

#include <algorithm>

namespace alpha::core {

Mixer::Mixer() : buses_{{"master", 1.0}, {"music", 1.0}, {"sfx", 1.0}, {"voice", 1.0}} {}

VoiceId Mixer::play(SoundPtr sound, PlayOptions options) {
  voices_.push_back({next_id_, std::move(sound), options, 0.0});
  return next_id_++;
}

void Mixer::stop(VoiceId id) {
  std::erase_if(voices_, [id](const Voice& v) { return v.id == id; });
}

void Mixer::stop_all() {
  voices_.clear();
  tracks_.clear();
}

bool Mixer::playing(VoiceId id) const {
  return std::any_of(voices_.begin(), voices_.end(), [id](const Voice& v) { return v.id == id; });
}

BusId Mixer::add_bus(const std::string& name) {
  if (const auto found = find_bus(name)) return *found;
  buses_.emplace_back(name, 1.0);
  return buses_.size() - 1;
}

std::optional<BusId> Mixer::find_bus(std::string_view name) const {
  for (BusId i = 0; i < buses_.size(); ++i) {
    if (buses_[i].first == name) return i;
  }
  return std::nullopt;
}

std::vector<std::string> Mixer::bus_names() const {
  std::vector<std::string> out;
  for (const auto& [name, volume] : buses_) out.push_back(name);
  return out;
}

void Mixer::music(std::string name, StreamPtr stream, MusicOptions options) {
  tracks_.clear();
  tracks_.emplace_back(std::move(name), std::move(stream), options);
}

void Mixer::crossfade(std::string name, StreamPtr stream, MusicOptions options, float seconds) {
  stop_music(seconds);
  options.fade_in = seconds;
  tracks_.emplace_back(std::move(name), std::move(stream), options);
}

void Mixer::stop_music(float fade_out) {
  for (MusicTrack& t : tracks_) t.fade_to(0.0f, fade_out);
  std::erase_if(tracks_, [](const MusicTrack& t) { return t.finished(); });
}

std::string Mixer::music_name() const {
  if (tracks_.empty() || tracks_.back().fading_out()) return "";
  return tracks_.back().name();
}

bool Mixer::render(Voice& voice, std::span<float> out) const {
  const std::vector<float>& samples = *voice.sound;
  const auto length = static_cast<double>(samples.size());
  const auto gain = static_cast<float>(voice.options.volume * bus_volume(voice.options.bus) * bus_volume(kMaster));
  const float left = gain * std::min(1.0f, 1.0f - voice.options.pan);
  const float right = gain * std::min(1.0f, 1.0f + voice.options.pan);
  for (std::size_t f = 0; f + 1 < out.size(); f += 2) {
    if (voice.position >= length) {
      if (!voice.options.loop || samples.empty()) return false;
      voice.position -= length;
    }
    const float sample = samples[static_cast<std::size_t>(voice.position)];
    out[f] += sample * left;
    out[f + 1] += sample * right;
    voice.position += voice.options.pitch;
  }
  return voice.options.loop || voice.position < length;
}

void Mixer::mix(std::span<float> out) {
  std::fill(out.begin(), out.end(), 0.0f);
  std::erase_if(voices_, [&](Voice& v) { return !render(v, out); });
  const auto music = static_cast<float>(bus_volume(kMusic) * bus_volume(kMaster));
  for (MusicTrack& t : tracks_) t.render(out, music);
  std::erase_if(tracks_, [](const MusicTrack& t) { return t.finished(); });
  for (float& sample : out) sample = std::clamp(sample, -1.0f, 1.0f);
}

}  // namespace alpha::core
