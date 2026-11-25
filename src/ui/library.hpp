#pragma once

#include <string>

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

// Registers the font file at `path` as font-family `family` (assets.fonts).
// Throws core::Error when the file is missing or not a font. Needs a Library.
void load_font(const std::string& path, const std::string& family);

}  // namespace alpha::ui
