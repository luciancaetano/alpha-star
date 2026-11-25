#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "core/os.hpp"

// What differs on phones (Android), kept to hints and the pack location so the
// rest of the engine runs unchanged. Desktop builds call the same functions:
// the hints only act on mobile platforms.
namespace alpha::platform {

// SDL_HINT_ORIENTATIONS for a game of this size: portrait when taller than
// wide, otherwise landscape (both ways round, following the sensor). The
// Android manifest written by `alpha export --target android-arm64` uses the same rule.
const char* orientations(int width, int height);

// Before the window opens: the orientation, and the Android back button
// delivered to the game as the "Back" input instead of closing the app.
void prepare_mobile(int width, int height);

// The OS this build runs on (ctx.platform), or the one `--platform` asked to preview.
core::Os running_os();
void preview_os(std::optional<core::Os> os);  // nullopt: the real one again

// The pack that ships with the engine, as a path for Host::load, or "" when
// there is none: game.jsbc or main.js next to the executable (`alpha export
// --standalone`); on Android the APK's assets (SDL reads relative paths from them).
std::string bundled_pack();

// The file a pack directory runs: its game.jsbc (compiled standalone export)
// when present, else its main.js. Bytecode is only ever taken from the pack itself.
std::filesystem::path pack_entry(const std::filesystem::path& dir);

}  // namespace alpha::platform
