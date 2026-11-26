#pragma once

#include <filesystem>
#include <vector>

#include "core/glyph_layout.hpp"

// TrueType / OpenType font sources: a font plus its fallback fonts (a chain).
// Each code point is drawn by the first font of the chain that has it (marks,
// joiners and spaces stay with the font of their run); what none has draws as
// a box, with a warning once per Unicode block. Runs are shaped by HarfBuzz.
namespace alpha::platform {

// Throws core::Error naming the file; fallbacks must be TrueType / OpenType
// sources (their own fallbacks are not followed).
core::FontSourcePtr load_truetype(const std::filesystem::path& file, bool crisp,
                                  const std::vector<core::FontSourcePtr>& fallbacks);

}  // namespace alpha::platform
