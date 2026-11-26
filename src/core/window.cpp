#include "core/window.hpp"

#include <algorithm>

namespace alpha::core {

std::optional<Scaling> parse_scaling(std::string_view name) {
  if (name == "letterbox") return Scaling::Letterbox;
  if (name == "integer") return Scaling::Integer;
  if (name == "stretch") return Scaling::Stretch;
  if (name == "expand") return Scaling::Expand;
  if (name == "crop") return Scaling::Crop;
  return std::nullopt;
}

Vec2 expanded_size(Vec2 game, Vec2 output) {
  if (output.x <= 0 || output.y <= 0) return game;
  const float aspect = output.x / output.y;
  if (aspect > game.x / game.y) return {game.y * aspect, game.y};
  return {game.x, game.x / aspect};
}

Vec2 initial_window_size(Vec2 game, Scaling scaling) { return game * (scaling == Scaling::Integer ? 2.0f : 1.5f); }

bool starts_fullscreen(const WindowConfig& config, Os os) { return config.fullscreen || is_mobile(os); }

Insets safe_insets(const PixelGrid& grid, Vec2 view, Vec2 safe_min, Vec2 safe_max) {
  const auto logical = [&grid](Vec2 p) {
    return Vec2{(p.x - grid.offset.x) / grid.scale.x, (p.y - grid.offset.y) / grid.scale.y};
  };
  const Vec2 lo = logical(safe_min);
  const Vec2 hi = logical(safe_max);
  return {std::max(0.0f, lo.x), std::max(0.0f, lo.y), std::max(0.0f, view.x - hi.x), std::max(0.0f, view.y - hi.y)};
}

}  // namespace alpha::core
