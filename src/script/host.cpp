#include "script/host.hpp"

#include <algorithm>
#include <array>

#include "platform/mobile.hpp"
#include "platform/vfs.hpp"
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

bool flag_or(JSContext* ctx, const Value& obj, const char* name, bool fallback) {
  const Value v = obj.prop(name);
  return JS_IsUndefined(v.get()) ? fallback : JS_ToBool(ctx, v.get()) == 1;
}

core::Scaling scaling_of(JSContext* ctx, const Value& w) {
  const Value scaling = w.prop("scaling");
  if (JS_IsUndefined(scaling.get())) return core::Scaling::Letterbox;
  const std::string name = string_arg(ctx, scaling.get(), "config.window.scaling");
  const auto parsed = core::parse_scaling(name);
  if (!parsed) throw core::Error("config.window.scaling must be \"letterbox\", \"integer\", \"stretch\", \"expand\" or \"crop\", got \"" + name + "\"");
  return *parsed;
}

// config.window: { fullscreen?, scaling?, resizable?, altEnter?, borderless?, alwaysOnTop?, maximized?, vsync? }
core::WindowConfig window_config(JSContext* ctx, const Value& info) {
  const Value w = info.prop("window");
  core::WindowConfig c;
  if (JS_IsUndefined(w.get())) return c;
  if (!JS_IsObject(w.get())) throw core::Error("config.window must be { fullscreen?, scaling?, resizable?, altEnter?, borderless?, alwaysOnTop?, maximized?, vsync? }");
  c.fullscreen = flag_or(ctx, w, "fullscreen", false);
  c.resizable = flag_or(ctx, w, "resizable", true);
  c.alt_enter = flag_or(ctx, w, "altEnter", true);
  c.borderless = flag_or(ctx, w, "borderless", false);
  c.always_on_top = flag_or(ctx, w, "alwaysOnTop", false);
  c.maximized = flag_or(ctx, w, "maximized", false);
  c.vsync = flag_or(ctx, w, "vsync", true);
  c.scaling = scaling_of(ctx, w);
  return c;
}

GameInfo game_info(JSContext* ctx, const Value& info) {
  const Value title = info.prop("title");
  const Value size = info.prop("size");
  const core::Vec2 wh = vec2_arg(ctx, size.get(), "config.size");
  if (wh.x < 1 || wh.y < 1) throw core::Error("config.size must be positive");
  GameInfo out;
  out.title = to_string(ctx, title.get());
  out.width = static_cast<int>(wh.x);
  out.height = static_cast<int>(wh.y);
  out.background = color_field(ctx, info, "background", "config.background");
  out.window = window_config(ctx, info);
  out.pause_on_focus_loss = flag_or(ctx, info, "pauseOnFocusLoss", true);
  return out;
}

}  // namespace

Host::Host(SavePathFor save_path_for)
    : save_path_for_(std::move(save_path_for)), vm_(std::make_unique<Vm>(save_path_for_, log_)) {}

Host::~Host() = default;

GameInfo Host::load(const std::string& path) {
  // A pack directory (from `alpha export`) or its main.js directly.
  const std::filesystem::path pack(path);
  main_path_ = platform::vfs::is_directory(pack) ? platform::pack_entry(pack).string() : path;
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
  GameInfo info = game_info(vm.engine->ctx(), vm.engine->call(vm.runtime("boot"), args));
  info.shaders = !vm.main().shaders.empty();
  return info;
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
  collect_times();
}

// The step's natives timed their sections; the script is the rest of the call.
void Host::collect_times() {
  core::FrameTimes& spent = vm_->main().times;
  frame_times_[core::Section::script] += std::max(0.0, vm_->engine->last_call_ms() - spent.total());
  for (std::size_t i = 0; i < core::kSections; ++i) frame_times_.ms[i] += spent.ms[i];
  spent = {};
}

void Host::end_frame() {
  perf_.push(frame_times_);
  frame_times_ = {};
}

std::size_t Host::entity_count() {
  const ecs::World* w = world();
  return w ? w->entities().size() : 0;
}

void Host::set_app_paused(bool paused) {
  app_paused_ = paused;
  notify("appPaused", JS_NewBool(vm_->engine->ctx(), paused));
}

void Host::low_memory() {
  log_.push(core::LogLevel::Warn, "the system is low on memory");
  notify("lowMemory", JS_UNDEFINED);
}

void Host::notify(const char* name, JSValue arg) {
  if (paused()) return;
  try {
    std::array<JSValueConst, 1> args{arg};
    vm_->engine->call(vm_->runtime(name), args);
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
  const core::ScopedTime t(frame_times_[core::Section::render]);  // building the draw list
  render::Frame f{info_.background, {}, {}};
  if (const ecs::World* world = vm_->main().world.get()) {
    f.shapes = world->draw_items(blend);
    f.view = world->view(blend);
    f.lights = world->lights(blend);
    f.effects = world->postfx().chain();
  }
  f.lighting = vm_->main().lighting;
  if (paused()) f.texts = error_overlay(error_, static_cast<float>(info_.width));
#if ALPHA_DEBUG_TOOLS
  else if (!reload_error_.empty()) f.texts = error_overlay(reload_error_, static_cast<float>(info_.width));
#endif
  return f;
}

void Host::resize_view(core::Vec2 size) {
  ScriptState& s = vm_->main();
  if (s.view_size == size) return;
  s.view_size = size;
  if (s.world) s.world->resize_view(size);
  if (s.ui) s.ui->resize(static_cast<int>(size.x), static_cast<int>(size.y));
}

bool Host::reaches_game(const SDL_Event& event) {
  ui::Ui* game_ui = vm_->main().ui.get();
  return paused() || !game_ui || !ui::consumes(*game_ui, event);
}

void Host::draw_ui(SDL_Renderer* sdl, double now) {
  ui::Ui* game_ui = vm_->main().ui.get();
  if (!game_ui || paused()) return;  // the error overlay must stay readable
  const core::ScopedTime t(frame_times_[core::Section::ui]);
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
    if (app_paused_) set_app_paused(true);  // the new game knows it is paused
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
