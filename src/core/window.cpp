#include "core/window.hpp"

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

}  // namespace alpha::core
