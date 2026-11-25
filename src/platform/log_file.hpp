#pragma once

#include <string>

#include "core/log_ring.hpp"

namespace alpha::platform {

// <per-user data dir>/<name>, e.g. %APPDATA%/alpha-star/<title>/save.json.
// The game folder may be read-only or shared, so nothing is written next to it.
// Empty (and logged) when the platform has no such dir.
std::string pref_file(const std::string& title, const char* name);

// Every line of `ring`, oldest first, in format_line form.
std::string text_of(const core::LogRing& ring);

// Shipping rule: the player has no console, so stderr (console.*, script
// errors) and SDL_Log go to `path` (pref_file(title, "log.txt"), truncated per
// run), starting with the lines logged before it opened. False if it cannot open.
bool redirect_logs(const std::string& path, const core::LogRing& earlier);

}  // namespace alpha::platform
