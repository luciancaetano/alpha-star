#pragma once

#include <string>
#include <vector>

#include "core/glyph_layout.hpp"

namespace alpha::ui {

// RmlUi is process-global (fonts, style sheets, factories). Every game UI
// holds a Library: the first one initialises RmlUi, the last one shuts it
// down, so tests and simulate() runs never leak UI state into each other.
class Library {
 public:
  Library();
  ~Library();
  Library(const Library&) = delete;
  Library& operator=(const Library&) = delete;
};

// Warnings and errors RmlUi logged since the last call (for agent-readable
// errors, e.g. why a document failed to load). Empty when there were none.
std::string take_messages();

// Registers the font file at `path` (.ttf/.otf, or a BMFont .fnt with its
// pages) as font-family `family` (assets.fonts) and returns it for world text.
// pixel_mode: crisp (pixel art) or smooth; fallbacks: loaded fonts drawing
// what it lacks (TrueType only). Throws core::Error when the file is missing
// or not a font. Needs a Library.
core::FontSourcePtr load_font(const std::string& path, const std::string& family, bool pixel_mode,
                              const std::vector<core::FontSourcePtr>& fallbacks);
// Output pixels per UI pixel (the renderer's logical presentation scale x
// high-DPI). On a change every face is made again at the new density.
void set_density(float density);

}  // namespace alpha::ui
