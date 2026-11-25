#include "script/host.hpp"

#include <array>

#include "script/asset_bindings.hpp"
#include "script/embedded_js.hpp"
#include "script/error_overlay.hpp"
#include "script/system_bindings.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

core::Color color_field(JSContext* ctx, const Value& obj, const char* name, const std::string& what) {
  const Value v = obj.prop(name);
  const std::string text = string_arg(ctx, v.get(), what);
  const auto color = core::parse_color(text);
  if (!color) throw core::Error(what + " must be a color like \"#ffffff\", got \"" + text + "\"");
  return *color;
}

core::Align align_field(JSContext* ctx, const Value& obj) {
  const Value v = obj.prop("align");
  const std::string name = string_arg(ctx, v.get(), "ui.text align");
  if (name == "center") return core::Align::Center;
  if (name == "right") return core::Align::Right;
  if (name == "left") return core::Align::Left;
  throw core::Error("ui.text align must be \"left\", \"center\" or \"right\", got \"" + name + "\"");
}

float number_field(JSContext* ctx, const Value& obj, const char* name, const std::string& what) {
  const Value v = obj.prop(name);
  return static_cast<float>(number_arg(ctx, v.get(), what));
}

core::TextItem text_item(JSContext* ctx, const Value& obj) {
  const Value text = obj.prop("text");
  return {to_string(ctx, text.get()),
          {number_field(ctx, obj, "x", "ui.text pos[0]"), number_field(ctx, obj, "y", "ui.text pos[1]")},
          number_field(ctx, obj, "size", "ui.text size"),
          color_field(ctx, obj, "color", "ui.text color"),
          align_field(ctx, obj)};
}

GameInfo game_info(JSContext* ctx, const Value& info) {
  const Value title = info.prop("title");
  const Value size = info.prop("size");
  const core::Vec2 wh = vec2_arg(ctx, size.get(), "config.size");
  if (wh.x < 1 || wh.y < 1) throw core::Error("config.size must be positive");
  return {to_string(ctx, title.get()), static_cast<int>(wh.x), static_cast<int>(wh.y),
          color_field(ctx, info, "background", "config.background")};
}

}  // namespace

Host::Host(SavePathFor save_path_for) : engine_(std::make_unique<Engine>()) {
  state_.save_path_for = std::move(save_path_for);
  JSContext* ctx = engine_->ctx();
  JS_SetContextOpaque(ctx, &state_);
  engine_->add_builtin("alpha", embedded::kAlpha);
  engine_->add_builtin("alpha/test", embedded::kAlphaTest);
  engine_->add_builtin("alpha:runtime", embedded::kRuntime);
  const Value global(ctx, JS_GetGlobalObject(ctx));
  Value natives(ctx, JS_NewObject(ctx));
  install_world_bindings(ctx, natives.get());
  install_system_bindings(ctx, natives.get());
  install_asset_bindings(ctx, natives.get());
  JS_SetPropertyStr(ctx, global.get(), "__native", natives.release());
}

Host::~Host() = default;

GameInfo Host::load(const std::string& path) {
  try {
    info_ = boot(path);
  } catch (const std::exception& e) {
    pause(e);
  }
  return info_;
}

GameInfo Host::boot(const std::string& target) {
  // A pack directory (from `alpha export`) or its main.js directly.
  const std::filesystem::path pack(target);
  const std::string path = std::filesystem::is_directory(pack) ? (pack / "main.js").string() : target;
  state_.asset_dir = std::filesystem::path(path).parent_path();
  const Value runtime = engine_->import("alpha:runtime");
  step_fn_.emplace(runtime.prop("step"));
  texts_fn_.emplace(runtime.prop("frameTexts"));
  const Value game = engine_->import(path).prop("default");
  std::array<JSValueConst, 1> args{game.get()};
  return game_info(engine_->ctx(), engine_->call(runtime.prop("boot"), args));
}

void Host::step(const core::InputSet& held, float dt) {
  if (paused()) return;
  state_.actions.update(held);
  try {
    std::array<JSValueConst, 1> args{JS_NewFloat64(engine_->ctx(), dt)};
    engine_->call(*step_fn_, args);
  } catch (const std::exception& e) {
    pause(e);
  }
}

std::vector<core::TextItem> Host::texts() {
  JSContext* ctx = engine_->ctx();
  const Value list = engine_->call(*texts_fn_, {});
  int64_t length = 0;
  JS_GetLength(ctx, list.get(), &length);
  std::vector<core::TextItem> items;
  for (int64_t i = 0; i < length; ++i) items.push_back(text_item(ctx, list.at(static_cast<std::uint32_t>(i))));
  return items;
}

void Host::pause(const std::exception& e) {
  error_ = e.what();
  state_.mixer.stop_all();  // a paused game is silent
}

render::Frame Host::frame() {
  render::Frame f{info_.background, {}, {}};
  if (state_.world) f.shapes = state_.world->draw_items();
  try {
    if (!paused()) f.texts = texts();
  } catch (const std::exception& e) {
    pause(e);
  }
  if (paused()) f.texts = error_overlay(error_, static_cast<float>(info_.width));
  return f;
}

}  // namespace alpha::script
