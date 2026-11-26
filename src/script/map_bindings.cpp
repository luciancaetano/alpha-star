#include "script/map_bindings.hpp"

#include <array>
#include <memory>
#include <string>

#include "core/error.hpp"
#include "core/tilemap.hpp"
#include "platform/vfs.hpp"
#include "script/effect_parser.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/sprite_bindings.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

// readText(file, optional): a text asset (LDtk .ldtk / .ldtkl, the pack's alpha-atlas.json),
// relative to the asset dir; null for a missing optional file.
JSValue read_text(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string file = string_arg(ctx, argv[0], "file");
  const auto text = platform::vfs::read_text(platform::vfs::in_pack(s.asset_dir, file));
  if (!text && JS_ToBool(ctx, argv[1]) == 1) return JS_NULL;
  if (!text) throw core::Error("cannot read \"" + file + "\" (paths are relative to the game's main file)");
  return JS_NewStringLen(ctx, text->data(), text->size());
}

constexpr std::size_t kCellStride = 6;

// Flat [dx, dy, sx, sy, flips, alpha, ...]: cell top-left in the layer, its tile's top-left
// in the tileset, flips (bit 0 = x, bit 1 = y, as LDtk's `f`) and opacity 0..1.
core::TileGridPtr grid_of(JSContext* ctx, JSValueConst tile, JSValueConst cells) {
  auto grid = std::make_shared<core::TileGrid>();
  grid->tile = vec2_arg(ctx, tile, "tile layer tile");
  const auto n = numbers_arg(ctx, cells, "tile layer cells");
  if (n.size() % kCellStride != 0) throw core::Error("tile layer cells must be [dx, dy, sx, sy, flips, alpha, ...]");
  for (std::size_t i = 0; i < n.size(); i += kCellStride) {
    const auto flips = static_cast<int>(n[i + 4]);
    grid->tiles.push_back({{static_cast<float>(n[i]), static_cast<float>(n[i + 1])},
                           {static_cast<int>(n[i + 2]), static_cast<int>(n[i + 3]), static_cast<int>(grid->tile.x),
                            static_cast<int>(grid->tile.y)},
                           (flips & 1) != 0, (flips & 2) != 0, static_cast<float>(n[i + 5])});
  }
  return grid;
}

// tileLayer({image, tile: [w, h], size: [w, h], pos: [x, y], z, cells, layer?, filter?}) -> entity id.
// One entity draws the whole layer (one batch, culled per cell); `pos` is its top-left.
JSValue tile_layer(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const Value spec(ctx, JS_DupValue(ctx, argv[0]));
  const core::Vec2 size = vec2_arg(ctx, spec.prop("size").get(), "tile layer size");
  core::EntitySpec e;
  e.pos = vec2_arg(ctx, spec.prop("pos").get(), "tile layer pos") + size * 0.5f;
  e.z = static_cast<int>(number_arg(ctx, spec.prop("z").get(), "tile layer z"));
  e.layer = parse_layer(ctx, spec.get(), s);
  e.shape = core::ShapeSpec{core::Rect{size}, {}, {}, decoded_image(s, string_arg(ctx, spec.prop("image").get(), "tileset image"), filter_arg(ctx, spec.prop("filter").get()))};
  e.shape->tiles = grid_of(ctx, spec.prop("tile").get(), spec.prop("cells").get());
  return JS_NewUint32(ctx, world_of(s).spawn(e));
}

// tileCells(id, tile, cells): replaces a tile layer entity's cells in place (keeps its draw order).
JSValue tile_cells(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::ShapeSpec& look = world_of(s).look(id_arg(ctx, argv[0]));
  if (!look.tiles) throw core::Error("tileCells: entity is not a tile layer");
  look.tiles = grid_of(ctx, argv[1], argv[2]);
  return JS_UNDEFINED;
}

// mergeCells(width, solid[]) -> flat [x, y, w, h, ...] in cells (core::merge_cells).
JSValue merge(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  const auto width = static_cast<int>(number_arg(ctx, argv[0], "layer width"));
  std::vector<bool> solid;
  for (const double v : numbers_arg(ctx, argv[1], "layer cells")) solid.push_back(v != 0.0);
  JSValue out = JS_NewArray(ctx);
  std::uint32_t i = 0;
  for (const core::Region& r : core::merge_cells(solid, width)) {
    for (const int v : {r.x, r.y, r.w, r.h}) JS_SetPropertyUint32(ctx, out, i++, JS_NewInt32(ctx, v));
  }
  return out;
}

constexpr std::array<Native, 4> kNatives{{
    {"readText", guarded<read_text>, 2},
    {"tileLayer", guarded<tile_layer>, 1},
    {"tileCells", guarded<tile_cells>, 3},
    {"mergeCells", guarded<merge>, 2},
}};

}  // namespace

void install_map_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
