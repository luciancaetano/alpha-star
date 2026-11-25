#include "script/sprite_bindings.hpp"

#include <array>
#include <memory>

#include "core/error.hpp"
#include "platform/assets.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

core::ImagePtr decoded_image(ScriptState& s, const std::string& file, bool smooth) {
  const std::string path = (s.asset_dir / file).string();
  core::ImagePtr& slot = s.decoded[smooth ? path + "|smooth" : path];  // one texture per filtering
  if (slot) return slot;
  core::Image image = platform::load_image(path);
  image.smooth = smooth;
  slot = std::make_shared<const core::Image>(std::move(image));
  return slot;
}

bool smooth_arg(JSContext* ctx, JSValueConst v) {
  if (JS_IsUndefined(v)) return false;
  if (!JS_IsBool(v)) throw core::Error("pixelMode must be true or false, got " + to_string(ctx, v));
  return JS_ToBool(ctx, v) == 0;
}

namespace {

int whole(double v, const std::string& what) {
  if (v < 0 || v != static_cast<double>(static_cast<int>(v))) {
    throw core::Error(what + " must be a whole number >= 0, got " + std::to_string(v));
  }
  return static_cast<int>(v);
}

// file, and its [x, y, w, h] place in it (an atlas page) or undefined = all of it.
core::Picture picture(JSContext* ctx, ScriptState& s, JSValueConst* argv, const std::string& what, bool smooth) {
  const std::string file = string_arg(ctx, argv[1], what);
  const core::ImagePtr image = decoded_image(s, file, smooth);
  core::Region r{0, 0, image->width, image->height};
  if (JS_IsUndefined(argv[2])) return {image, r};
  const auto n = numbers_arg(ctx, argv[2], what + " (atlas region)");
  if (n.size() != 4) throw core::Error(what + ": atlas region must be [x, y, w, h]");
  r = {whole(n[0], what), whole(n[1], what), whole(n[2], what), whole(n[3], what)};
  if (r.x + r.w > image->width || r.y + r.h > image->height) throw core::Error(what + ": atlas region outside " + file);
  return {image, r};
}

// loadImage(name, file, region?, pixelMode?)
JSValue load_image(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::Picture pic = picture(ctx, s, argv, "assets.images." + string_arg(ctx, argv[0], "image name"), smooth_arg(ctx, argv[3]));
  s.images.insert_or_assign(to_string(ctx, argv[0]), std::move(pic));
  return JS_UNDEFINED;
}

core::Animation animation(JSContext* ctx, const Value& spec, int frames, const std::string& what) {
  core::Animation a;
  for (const double f : numbers_arg(ctx, spec.prop("frames").get(), what + ".frames")) {
    const int index = whole(f, what + ".frames");
    if (index >= frames) {
      throw core::Error(what + ".frames: frame " + std::to_string(index) + " is outside the sheet (" +
                        std::to_string(frames) + " frames, 0.." + std::to_string(frames - 1) + ")");
    }
    a.frames.push_back(index);
  }
  if (a.frames.empty()) throw core::Error(what + ".frames must list at least one frame");
  const Value fps = spec.prop("fps");
  if (!JS_IsUndefined(fps.get())) a.fps = static_cast<float>(number_arg(ctx, fps.get(), what + ".fps"));
  if (!(a.fps > 0.0f)) throw core::Error(what + ".fps must be > 0");
  const Value loop = spec.prop("loop");
  a.loop = JS_IsUndefined(loop.get()) || JS_ToBool(ctx, loop.get()) == 1;
  return a;
}

void frame_size(JSContext* ctx, core::Sheet& sheet, JSValueConst v, const std::string& what) {
  const core::Vec2 f = vec2_arg(ctx, v, what + ".frame");
  sheet.frame_w = whole(f.x, what + ".frame[0]");
  sheet.frame_h = whole(f.y, what + ".frame[1]");
  if (sheet.frame_count() < 1) {
    throw core::Error(what + ".frame [" + std::to_string(sheet.frame_w) + ", " + std::to_string(sheet.frame_h) +
                      "] does not fit the image (" + std::to_string(sheet.picture.region.w) + "x" +
                      std::to_string(sheet.picture.region.h) + ")");
  }
}

// loadSheet(name, file, region?, frame, [[animation, {frames, fps?, loop?}], ...], pixelMode?)
JSValue load_sheet(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string what = "assets.sheets." + string_arg(ctx, argv[0], "sheet name");
  auto sheet = std::make_shared<core::Sheet>();
  sheet->picture = picture(ctx, s, argv, what + ".image", smooth_arg(ctx, argv[5]));
  frame_size(ctx, *sheet, argv[3], what);
  const Value list(ctx, JS_DupValue(ctx, argv[4]));
  int64_t n = 0;
  JS_GetLength(ctx, list.get(), &n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const Value entry = list.at(i);
    const std::string anim = string_arg(ctx, entry.at(0).get(), what + " animation name");
    sheet->animations[anim] = animation(ctx, entry.at(1), sheet->frame_count(), what + ".animations." + anim);
  }
  s.sheets[to_string(ctx, argv[0])] = std::move(sheet);
  return JS_UNDEFINED;
}

// play(id, name, restart)
JSValue play(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world_of(s).play(id_arg(ctx, argv[0]), string_arg(ctx, argv[1], "animation"), JS_ToBool(ctx, argv[2]) == 1);
  return JS_UNDEFINED;
}

JSValue animation_of(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = world_of(s).animation(id_arg(ctx, argv[0]));
  return JS_NewStringLen(ctx, name.data(), name.size());
}

// flip(id) -> [x, y]; flip(id, x, y) sets them first.
JSValue flip(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ecs::World& w = world_of(s);
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  if (!JS_IsUndefined(argv[1])) w.set_flip(id, JS_ToBool(ctx, argv[1]) == 1, JS_ToBool(ctx, argv[2]) == 1);
  const auto [x, y] = w.flip(id);
  JSValue out = JS_NewArray(ctx);
  JS_SetPropertyUint32(ctx, out, 0, JS_NewBool(ctx, x));
  JS_SetPropertyUint32(ctx, out, 1, JS_NewBool(ctx, y));
  return out;
}

constexpr std::array<Native, 5> kNatives{{
    {"loadImage", guarded<load_image>, 4}, {"loadSheet", guarded<load_sheet>, 6},
    {"play", guarded<play>, 3},            {"animation", guarded<animation_of>, 1},
    {"flip", guarded<flip>, 3},
}};

}  // namespace

void install_sprite_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
