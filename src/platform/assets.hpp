#pragma once

#include <string>
#include <vector>

#include "core/image.hpp"

namespace alpha::platform {

// Decodes PNG, JPG or BMP to RGBA8. Throws core::Error naming the file and the reason.
core::Image load_image(const std::string& path);

}  // namespace alpha::platform
