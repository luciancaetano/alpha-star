#pragma once

#include <string>
#include <vector>

#include "core/image.hpp"

namespace alpha::platform {

// Decodes a WAV file to mono float samples at core::Mixer::kSampleRate.
// Throws core::Error naming the file and the reason.
std::vector<float> load_wav(const std::string& path);

// Decodes PNG, JPG or BMP to RGBA8. Throws core::Error naming the file and the reason.
core::Image load_image(const std::string& path);

}  // namespace alpha::platform
