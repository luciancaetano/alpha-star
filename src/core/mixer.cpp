#include "core/mixer.hpp"

#include <algorithm>

namespace alpha::core {

VoiceId Mixer::play(SoundPtr sound, PlayOptions options) {
  voices_.push_back({next_id_, std::move(sound), options, 0.0});
  return next_id_++;
}

void Mixer::stop(VoiceId id) {
  std::erase_if(voices_, [id](const Voice& v) { return v.id == id; });
}

void Mixer::stop_all() { voices_.clear(); }

bool Mixer::playing(VoiceId id) const {
  return std::any_of(voices_.begin(), voices_.end(), [id](const Voice& v) { return v.id == id; });
}

bool Mixer::render(Voice& voice, std::span<float> out) const {
  const std::vector<float>& samples = *voice.sound;
  const auto length = static_cast<double>(samples.size());
  const float gain = voice.options.volume * master_;
  for (float& sample : out) {
    if (voice.position >= length) {
      if (!voice.options.loop || samples.empty()) return false;
      voice.position -= length;
    }
    sample += samples[static_cast<std::size_t>(voice.position)] * gain;
    voice.position += voice.options.pitch;
  }
  return voice.options.loop || voice.position < length;
}

void Mixer::mix(std::span<float> out) {
  std::fill(out.begin(), out.end(), 0.0f);
  std::erase_if(voices_, [&](Voice& v) { return !render(v, out); });
  for (float& sample : out) sample = std::clamp(sample, -1.0f, 1.0f);
}

}  // namespace alpha::core
