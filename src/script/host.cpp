#include "script/host.hpp"

#include <array>

#include "script/error_overlay.hpp"
#include "script/input_bindings.hpp"
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
    : save_path_for_(std::move(save_path_for)), vm_(std::make_unique<Vm>(save_path_for_, log_)) {}

Host::~Host() = default;

GameInfo Host::load(const std::string& path) {
  // A pack directory (from `alpha export`) or its main.js directly.
  const std::filesystem::path pack(path);
  main_path_ = std::filesystem::is_directory(pack) ? (pack / "main.js").string() : path;
  pack_dir_ = std::filesystem::path(main_path_).parent_path();
  try {
    info_ = boot(*vm_, main_path_, "");
  } catch (const std::exception& e) {
    pause(e);
  }
  return info_;
}

GameInfo Host::boot(Vm& vm, const std::string& path, const std::string& from) {
  vm.main().asset_dir = std::filesystem::path(path).parent_path();
  vm.step_fn.emplace(vm.runtime("step"));
  vm.scene_fn.emplace(vm.runtime("sceneName"));
  const Value game = vm.import_bundle(path).prop("default");
  Value from_text(vm.engine->ctx(), JS_NewStringLen(vm.engine->ctx(), from.data(), from.size()));
  std::array<JSValueConst, 2> args{game.get(), from_text.get()};
  return game_info(vm.engine->ctx(), vm.engine->call(vm.runtime("boot"), args));
}

void Host::step(const core::InputFrame& input, float dt) {
  if (paused()) return;
  vm_->main().controls.update(input);
  if (vm_->main().world) vm_->main().world->snapshot();
  try {
    std::array<JSValueConst, 1> args{JS_NewFloat64(vm_->engine->ctx(), dt)};
    vm_->engine->call(*vm_->step_fn, args);
    step_ms_ = vm_->engine->last_call_ms();
  } catch (const std::exception& e) {
    pause(e);
  }
}

core::RumbleRequests Host::take_rumble() { return vm_->main().controls.take_rumble(); }

void Host::pause(const std::exception& e) {
  error_ = vm_->located(e);
  log_.push(core::LogLevel::Error, error_);
  vm_->main().mixer.stop_all();  // a paused game is silent
}

render::Frame Host::frame(float blend) {
  render::Frame f{info_.background, {}, {}};
  if (vm_->main().world) f.shapes = vm_->main().world->draw_items(blend);
  if (paused()) f.texts = error_overlay(error_, static_cast<float>(info_.width));
#if ALPHA_DEBUG_TOOLS
  else if (!reload_error_.empty()) f.texts = error_overlay(reload_error_, static_cast<float>(info_.width));
#endif
  return f;
}

bool Host::reaches_game(const SDL_Event& event) {
  ui::Ui* game_ui = vm_->main().ui.get();
  return paused() || !game_ui || !ui::consumes(*game_ui, event);
}

void Host::draw_ui(SDL_Renderer* sdl, double now) {
  ui::Ui* game_ui = vm_->main().ui.get();
  if (!game_ui || paused()) return;  // the error overlay must stay readable
  game_ui->tick(now);
  game_ui->render(sdl);
}

void Host::release_ui() {
  if (ui::Ui* game_ui = vm_->main().ui.get()) game_ui->attach(nullptr);
}

#if ALPHA_DEBUG_TOOLS
std::vector<TestResult> Host::test(const std::string& path, const std::string& asset_dir) {
  try {
    return run_tests(path, asset_dir);
  } catch (const std::exception& e) {
    return {{"(loading " + path + ")", vm_->located(e)}};
  }
}

std::vector<TestResult> Host::run_tests(const std::string& path, const std::string& asset_dir) {
  JSContext* ctx = vm_->engine->ctx();
  vm_->main().asset_dir = asset_dir.empty() ? std::filesystem::path(path).parent_path() : std::filesystem::path(asset_dir);
  const Value names_fn = vm_->engine->import("alpha/test").prop("testNames");
  const Value tests = vm_->import_bundle(path);
  std::array<JSValueConst, 1> args{tests.get()};
  const Value names = vm_->engine->call(names_fn, args);
  std::vector<TestResult> results;
  for (const std::string& name : strings_arg(ctx, names.get(), "test names")) {
    TestResult result{name, ""};
    try {
      vm_->engine->call(tests.prop(name.c_str()), {});
    } catch (const std::exception& e) {
      result.error = vm_->located(e);
    }
    results.push_back(std::move(result));
  }
  return results;
}

std::string Host::carried_scene() {
  try {
    const Value text = vm_->engine->call(vm_->runtime("carriedScene"), {});
    return to_string(vm_->engine->ctx(), text.get());
  } catch (const std::exception&) {
    return "";  // the old game cannot say where it was: the new one starts fresh
  }
}

std::string Host::reload() {
  auto next = std::make_unique<Vm>(save_path_for_, log_);
  try {
    const GameInfo info = boot(*next, main_path_, carried_scene());
    vm_ = std::move(next);
    info_ = info;
    error_.clear();
    reload_error_.clear();
  } catch (const std::exception& e) {
    reload_error_ = "reload failed; the previous game keeps running:\n" + next->located(e);
    log_.push(core::LogLevel::Error, reload_error_);
  }
  return reload_error_;
}
#endif

const core::LogRing& Host::log() const { return log_; }

std::string Host::scene() {
  if (!vm_->scene_fn) return "";
  const Value name = vm_->engine->call(*vm_->scene_fn, {});
  return JS_IsString(name.get()) ? to_string(vm_->engine->ctx(), name.get()) : "";
}

ecs::World* Host::world() { return vm_->main().world.get(); }

const core::TagRegistry& Host::tags() { return vm_->main().tags; }


}  // namespace alpha::script
