#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/color.hpp"
#include "core/image.hpp"
#include "core/sheet.hpp"
#include "core/tags.hpp"
#include "core/vec2.hpp"

// Plain data shared by script layer, ECS, physics and render. No behavior.
namespace alpha::core {

struct Circle {
  float radius = 0.0f;
};
struct Rect {
  Vec2 size;
};
using Geometry = std::variant<Circle, Rect>;

enum class BodyType { Static, Kinematic, Dynamic };

struct ColliderSpec {
  Geometry geometry;
  bool sensor = false;
};

struct BodySpec {
  BodyType type = BodyType::Static;
  ColliderSpec collider;
  Vec2 velocity;
};

// One cell of a tile layer: where it goes (top-left, relative to the layer's
// top-left) and which part of the tileset image it shows.
struct TileQuad {
  Vec2 pos;
  Region src;
};

// A whole tile layer drawn as one entity (one batch, culled per tile).
struct TileGrid {
  Vec2 tile;  // cell size in pixels
  std::vector<TileQuad> tiles;
};
using TileGridPtr = std::shared_ptr<const TileGrid>;

// A sprite-sheet animation to start the entity with ("" = show frame 0, play later).
struct AnimationSpec {
  SheetPtr sheet;
  std::string name;
};

// Visual of an entity. With `image`, the `src` part of the image (empty =
// all of it) is drawn stretched to the geometry's Rect size, optionally
// mirrored (fill/outline unused); with `tiles` too, the image is a tileset and
// the Rect is the layer's size; with `animation`, a sheet plays into `src`.
// Otherwise a filled shape. The constructor takes the four basic fields in
// order; the sprite fields are set by name.
struct ShapeSpec {
  ShapeSpec(Geometry g = Circle{}, Color f = {}, std::optional<Color> o = {}, ImagePtr img = {});

  Geometry geometry;
  Color fill;
  std::optional<Color> outline;
  ImagePtr image;
  Region src;
  bool flip_x = false;
  bool flip_y = false;
  TileGridPtr tiles;
  std::optional<AnimationSpec> animation;
};

struct EntitySpec {
  TagSet tags;
  Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
  std::optional<ShapeSpec> shape;
  std::optional<BodySpec> body;
};

// One shape to draw, already resolved to world placement.
struct DrawItem {
  ShapeSpec shape;
  Vec2 pos;
  float rotation = 0.0f;  // degrees
};

enum class Align { Left, Center, Right };

// One line of text; pos.y is the vertical center, pos.x depends on align.
struct TextItem {
  std::string text;
  Vec2 pos;
  float size = 16.0f;
  Color color{255, 255, 255, 255};
  Align align = Align::Left;
};

}  // namespace alpha::core
