#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/color.hpp"
#include "core/image.hpp"
#include "core/lighting.hpp"
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

// Surface of a collider (Box2D defaults). density in kg per square meter (50 px = 1 m).
struct Material {
  float friction = 0.6f;
  float restitution = 0.0f;  // bounciness 0..1
  float density = 1.0f;
};

// `one_way`: solid only for bodies coming from above (smaller y); they pass up through it.
struct ColliderSpec {
  ColliderSpec(Geometry g = Circle{}, bool is_sensor = false);

  Geometry geometry;
  bool sensor = false;
  Material material;
  bool one_way = false;
};

// Collision filter bits (config.collision.categories). Two bodies collide when
// each one's category is in the other's mask. Bit 0 = bodies without a category.
struct CollisionFilter {
  std::uint64_t category = 1;
  std::uint64_t mask = ~std::uint64_t{0};
};

struct BodySpec {
  BodySpec(BodyType t = BodyType::Static, ColliderSpec c = {}, Vec2 v = {});

  BodyType type = BodyType::Static;
  ColliderSpec collider;
  Vec2 velocity;
  bool fixed_rotation = false;
  float gravity_scale = 1.0f;
  bool bullet = false;  // continuous collision against other moving bodies (fast projectiles)
  CollisionFilter filter;
};

// One cell of a tile layer: where it goes (top-left, relative to the layer's
// top-left) and which part of the tileset image it shows.
struct TileQuad {
  Vec2 pos;
  Region src;
  bool flip_x = false;
  bool flip_y = false;
  float alpha = 1.0f;  // times the shape's color (LDtk tile opacity x layer opacity)
};

// A whole tile layer drawn as one entity (one batch, culled per tile).
struct TileGrid {
  Vec2 tile;  // cell size in pixels; zero: each cell is its src size (text glyphs)
  std::vector<TileQuad> tiles;
};
using TileGridPtr = std::shared_ptr<const TileGrid>;

enum class Align { Left, Center, Right };

// Paragraph direction of text: auto takes it from the first strong letter
// (Arabic/Hebrew = right-to-left), as Unicode's bidi algorithm does.
enum class TextDir : std::uint8_t { Auto, Ltr, Rtl };

// How a shape's pixels combine with what is under them.
enum class Blend { Alpha, Add, Multiply };

class FontSource;
using FontSourcePtr = std::shared_ptr<const FontSource>;

// World text: drawn with `font` (an assets.fonts name) or, without one, the
// built-in 8x8 font, `size` pixels high; rasterized by the renderer at the
// output resolution (window pixels x camera zoom), so it stays sharp.
// `pos` is the anchor: align picks which end (or the middle) of the lines sits
// on it; the block of lines is vertically centered on it. `width` > 0 wraps
// lines at that many pixels (spaces, and between CJK characters); `dir` is
// the paragraph direction (mixed scripts are ordered by Unicode bidi).
struct TextLook {
  std::string text;
  std::string font;
  FontSourcePtr face;  // the font itself (set by the script layer)
  float size = 16.0f;
  Align align = Align::Center;
  TextDir dir = TextDir::Auto;
  float width = 0.0f;
};

// A sprite-sheet animation to start the entity with ("" = show frame 0, play later).
struct AnimationSpec {
  SheetPtr sheet;
  std::string name;
};

// Visual of an entity. With `image`, the `src` part of the image (empty =
// all of it) is drawn stretched to the geometry's Rect size, optionally
// mirrored (fill/outline unused); with `tiles` too, the image is a tileset and
// the Rect is the layer's size; with `animation`, a sheet plays into `src`.
// With `text`, the line is drawn in `fill` (glyph quads of a font atlas in
// image/tiles, or the debug font when there is no image).
// Otherwise a filled shape. Every look is multiplied by `tint` and `alpha`
// and blended with `blend`. The constructor takes the four basic fields in
// order; the others are set by name.
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
  Color tint{255, 255, 255, 255};
  float alpha = 1.0f;  // 0..1, multiplies tint.a
  Blend blend = Blend::Alpha;
  std::optional<TextLook> text;
  // Filled shapes only: a 1 px alpha-feathered edge (pixelMode false) instead of a hard one.
  bool smooth = false;
};

// A named draw layer from config.layers: drawn in `z` order (then entity z),
// seen through the camera scaled by `parallax` (0 = fixed to the screen,
// 1 = moves with the world). The unnamed default layer is {"", 0, 1}.
struct Layer {
  std::string name;
  int z = 0;
  float parallax = 1.0f;
};

// min..max, picked uniformly per particle (or start..end over a lifetime).
struct Span {
  float min = 0.0f;
  float max = 0.0f;
};

// A particle emitter attached to an entity: particles are born at the
// entity's position, then fly on their own (world space). `look` is one
// particle's shape at size 1 (circle, square, or an image region).
// `angle` in degrees, 0 = right, 90 = down. One-shot: the burst only, and
// the entity is destroyed once its last particle died.
struct EmitterSpec {
  float rate = 0.0f;  // particles per second
  int burst = 0;      // emitted on the first step
  Span lifetime{1.0f, 1.0f};
  Span speed{0.0f, 0.0f};
  Span angle{0.0f, 360.0f};
  Vec2 gravity;
  Span size{4.0f, 4.0f};  // start, end
  Color color_start{255, 255, 255, 255};
  Color color_end{255, 255, 255, 0};
  ShapeSpec look{Circle{0.5f}, {255, 255, 255, 255}};
  int max = 100;
  bool one_shot = false;
};

struct EntitySpec {
  TagSet tags;
  Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
  std::optional<ShapeSpec> shape;
  std::optional<BodySpec> body;
  Layer layer;
  std::optional<EmitterSpec> emitter;
  std::optional<LightSpec> light;
};

// One live particle as drawn: center, size and color (tint already applied).
struct ParticleQuad {
  Vec2 pos;
  float size = 0.0f;
  Color color;
};

// One shape to draw, already resolved to world placement. With particles,
// the shape is drawn once per particle (scaled to its size) instead.
struct DrawItem {
  DrawItem(ShapeSpec s = {}, Vec2 p = {}, float degrees = 0.0f);

  ShapeSpec shape;
  Vec2 pos;
  float rotation = 0.0f;  // degrees
  float parallax = 1.0f;  // of the item's layer
  std::vector<ParticleQuad> particles;
};

// One line of text; pos.y is the vertical center, pos.x depends on align.
struct TextItem {
  std::string text;
  Vec2 pos;
  float size = 16.0f;
  Color color{255, 255, 255, 255};
  Align align = Align::Left;
};

}  // namespace alpha::core
