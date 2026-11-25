#include "script/map_bindings.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "core/error.hpp"
#include "core/tilemap.hpp"
#include "script/effect_parser.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/sprite_bindings.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

// readText(file, optional): a text asset (Tiled .tmj / .tsj, the pack's alpha-atlas.json),
// relative to the asset dir; null for a missing optional file.
JSValue read_text(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string file = string_arg(ctx, argv[0], "file");
  std::ifstream in(s.asset_dir / file, std::ios::binary);
  if (!in && JS_ToBool(ctx, argv[1]) == 1) return JS_NULL;
  if (!in) throw core::Error("cannot read \"" + file + "\" (paths are relative to the game's main file)");
  std::ostringstream text;
  text << in.rdbuf();
  const std::string out = text.str();
  return JS_NewStringLen(ctx, out.data(), out.size());
}

// Flat [dx, dy, sx, sy, ...]: cell top-left in the layer, its tile's top-left in the tileset.
std::vector<core::TileQuad> cells(JSContext* ctx, const Value& spec, core::Vec2 tile) {
  const auto n = numbers_arg(ctx, spec.prop("cells").get(), "tile layer cells");
  std::vector<core::TileQuad> out;
  for (std::size_t i = 0; i + 3 < n.size(); i += 4) {
    out.push_back({{static_cast<float>(n[i]), static_cast<float>(n[i + 1])},
                   {static_cast<int>(n[i + 2]), static_cast<int>(n[i + 3]), static_cast<int>(tile.x),
                    static_cast<int>(tile.y)}});
  }
  return out;
}

// tileLayer({image, tile: [w, h], size: [w, h], pos: [x, y], z, cells, layer?}) -> entity id.
// One entity draws the whole layer (one batch, culled per cell); `pos` is its top-left.
JSValue tile_layer(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const Value spec(ctx, JS_DupValue(ctx, argv[0]));
  auto grid = std::make_shared<core::TileGrid>();
  grid->tile = vec2_arg(ctx, spec.prop("tile").get(), "tile layer tile");
  grid->tiles = cells(ctx, spec, grid->tile);
  const core::Vec2 size = vec2_arg(ctx, spec.prop("size").get(), "tile layer size");
  core::EntitySpec e;
  e.pos = vec2_arg(ctx, spec.prop("pos").get(), "tile layer pos") + size * 0.5f;
  e.z = static_cast<int>(number_arg(ctx, spec.prop("z").get(), "tile layer z"));
  e.layer = parse_layer(ctx, spec.get(), s);
  e.shape = core::ShapeSpec{core::Rect{size}, {}, {}, decoded_image(s, string_arg(ctx, spec.prop("image").get(), "tileset image"))};
  e.shape->tiles = std::move(grid);
  return JS_NewUint32(ctx, world_of(s).spawn(e));
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

constexpr std::array<Native, 3> kNatives{{
    {"readText", guarded<read_text>, 2},
    {"tileLayer", guarded<tile_layer>, 1},
    {"mergeCells", guarded<merge>, 2},
}};

}  // namespace

void install_map_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
