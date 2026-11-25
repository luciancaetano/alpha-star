#pragma once

#include <optional>
#include <string>
#include <variant>

#include "core/color.hpp"
#include "core/image.hpp"
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

// Visual of an entity. With `image`, the image is drawn stretched to the
// geometry's Rect size (fill/outline unused); otherwise a filled shape.
struct ShapeSpec {
  Geometry geometry;
  Color fill;
  std::optional<Color> outline;
  ImagePtr image;
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
