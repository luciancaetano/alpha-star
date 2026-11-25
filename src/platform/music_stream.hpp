#pragma once

#include <string>
#include <vector>

#include "core/music_track.hpp"

namespace alpha::platform {

// Opens `path` (.ogg, .mp3, .wav) as a core::Stream: decoded on demand in
// small chunks (kChunkFrames of the file's frames) and converted to stereo at
// core::Mixer::kSampleRate. Throws core::Error naming the file.
core::StreamPtr open_music(const std::string& path);

// Decodes a whole file to mono floats at core::Mixer::kSampleRate (sounds).
std::vector<float> load_sound(const std::string& path);

// Read-ahead per decode: ~21 ms at 48 kHz.
constexpr int kChunkFrames = 1024;

}  // namespace alpha::platform
