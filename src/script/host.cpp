#include "script/host.hpp"

#include <array>

#include "script/asset_bindings.hpp"
#include "script/bundle_map.hpp"
#include "script/embedded_js.hpp"
#include "script/error_overlay.hpp"
#include "script/input_bindings.hpp"
#include "script/instance_bindings.hpp"
#include "script/system_bindings.hpp"
#include "script/ui_bindings.hpp"
#include "script/world_bindings.hpp"
#include "ui/input.hpp"

namespace alpha::script {

namespace {

core::Color color_field(JSContext* ctx, const Value& obj, const char* name, const std::string& what) {
  const Value v = obj.prop(name);
  const std::string text = string_arg(ctx, v.get(), what);
  const auto color = core::parse_color(text);
  if (!color) throw core::Error(what + " must be a color like \"#ffffff\", got \"" + text + "\"");
  return *color;
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

Host::Host(SavePathFor save_path_for)
    : instances_(std::move(save_path_for)), engine_(std::make_unique<Engine>()) {
  JSContext* ctx = engine_->ctx();
  JS_SetContextOpaque(ctx, &instances_);
  engine_->add_builtin("alpha", embedded::kAlpha);
#if ALPHA_DEBUG_TOOLS  // test runner only; importing it elsewhere lists the builtins that exist
  engine_->add_builtin("alpha/test", embedded::kAlphaTest);
#endif
  engine_->add_builtin("alpha:runtime", embedded::kRuntime);
  const Value global(ctx, JS_GetGlobalObject(ctx));
  Value natives(ctx, JS_NewObject(ctx));
  install_world_bindings(ctx, natives.get());
  install_system_bindings(ctx, natives.get());
  install_input_bindings(ctx, natives.get());
  install_asset_bindings(ctx, natives.get());
  install_instance_bindings(ctx, natives.get());
  install_ui_bindings(ctx, natives.get());
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
  instances_.main().asset_dir = std::filesystem::path(path).parent_path();
  const Value runtime = engine_->import("alpha:runtime");
  step_fn_.emplace(runtime.prop("step"));
  scene_fn_.emplace(runtime.prop("sceneName"));
  const Value game = import_bundle(path).prop("default");
  std::array<JSValueConst, 1> args{game.get()};
  return game_info(engine_->ctx(), engine_->call(runtime.prop("boot"), args));
}

Value Host::import_bundle(const std::string& path) {
  bundle_path_ = path;
  source_map_ = load_bundle_map(engine_->ctx(), path);
  return engine_->import(path);
}

std::string Host::located(const std::exception& e) const {
  return source_map_ ? core::rewrite_locations(e.what(), bundle_path_, *source_map_) : e.what();
}

void Host::step(const core::InputFrame& input, float dt) {
  if (paused()) return;
  instances_.main().controls.update(input);
  if (instances_.main().world) instances_.main().world->snapshot();
  try {
    std::array<JSValueConst, 1> args{JS_NewFloat64(engine_->ctx(), dt)};
    engine_->call(*step_fn_, args);
    step_ms_ = engine_->last_call_ms();
  } catch (const std::exception& e) {
    pause(e);
  }
}

core::RumbleRequests Host::take_rumble() { return instances_.main().controls.take_rumble(); }

void Host::pause(const std::exception& e) {
  error_ = located(e);
  instances_.log().push(core::LogLevel::Error, error_);
  instances_.main().mixer.stop_all();  // a paused game is silent
}

render::Frame Host::frame(float blend) {
  render::Frame f{info_.background, {}, {}};
  if (instances_.main().world) f.shapes = instances_.main().world->draw_items(blend);
  if (paused()) f.texts = error_overlay(error_, static_cast<float>(info_.width));
  return f;
}

bool Host::reaches_game(const SDL_Event& event) {
  ui::Ui* game_ui = instances_.main().ui.get();
  return paused() || !game_ui || !ui::consumes(*game_ui, event);
}

void Host::draw_ui(SDL_Renderer* sdl, double now) {
  ui::Ui* game_ui = instances_.main().ui.get();
  if (!game_ui || paused()) return;  // the error overlay must stay readable
  game_ui->tick(now);
  game_ui->render(sdl);
}

void Host::release_ui() {
  if (ui::Ui* game_ui = instances_.main().ui.get()) game_ui->attach(nullptr);
}

#if ALPHA_DEBUG_TOOLS
std::vector<TestResult> Host::test(const std::string& path, const std::string& asset_dir) {
  try {
    return run_tests(path, asset_dir);
  } catch (const std::exception& e) {
    return {{"(loading " + path + ")", located(e)}};
  }
}

std::vector<TestResult> Host::run_tests(const std::string& path, const std::string& asset_dir) {
  JSContext* ctx = engine_->ctx();
  instances_.main().asset_dir = asset_dir.empty() ? std::filesystem::path(path).parent_path() : std::filesystem::path(asset_dir);
  const Value names_fn = engine_->import("alpha/test").prop("testNames");
  const Value tests = import_bundle(path);
  std::array<JSValueConst, 1> args{tests.get()};
  const Value names = engine_->call(names_fn, args);
  std::vector<TestResult> results;
  for (const std::string& name : strings_arg(ctx, names.get(), "test names")) {
    TestResult result{name, ""};
    try {
      engine_->call(tests.prop(name.c_str()), {});
    } catch (const std::exception& e) {
      result.error = located(e);
    }
    results.push_back(std::move(result));
  }
  return results;
}
#endif

const core::LogRing& Host::log() const { return instances_.log(); }

std::string Host::scene() {
  if (!scene_fn_) return "";
  const Value name = engine_->call(*scene_fn_, {});
  return JS_IsString(name.get()) ? to_string(engine_->ctx(), name.get()) : "";
}

ecs::World* Host::world() { return instances_.main().world.get(); }

const core::TagRegistry& Host::tags() { return instances_.main().tags; }


}  // namespace alpha::script
