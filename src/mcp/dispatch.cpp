#include "mcp/dispatch.hpp"

#include <climits>
#include <cmath>

#include "core/error.hpp"
#include "core/input.hpp"
#include "core/version.hpp"
#include "mcp/base64.hpp"
#include "mcp/state_json.hpp"

namespace alpha::mcp {

namespace {

const json::Field* find(const json::Object& r, std::string_view key) {
  const auto it = r.find(key);
  return it == r.end() || std::holds_alternative<std::nullptr_t>(it->second) ? nullptr : &it->second;
}

std::string text(const json::Object& r, std::string_view key) {
  const json::Field* f = find(r, key);
  const auto* s = f ? std::get_if<std::string>(f) : nullptr;
  if (!s) throw core::Error("missing \"" + std::string(key) + "\": a string field");
  return *s;
}

std::optional<double> number(const json::Object& r, std::string_view key) {
  const json::Field* f = find(r, key);
  if (!f) return std::nullopt;
  const auto* d = std::get_if<double>(f);
  if (!d) throw core::Error("\"" + std::string(key) + "\" must be a number");
  return *d;
}

// An integer field in lo..hi; `fallback` when absent.
int integer(const json::Object& r, std::string_view key, int fallback, int lo, int hi) {
  const auto v = number(r, key);
  if (!v) return fallback;
  const std::string range = std::to_string(lo) + ".." + std::to_string(hi);
  if (*v != std::floor(*v)) throw core::Error("\"" + std::string(key) + "\" must be an integer " + range);
  if (*v < lo || *v > hi) throw core::Error("\"" + std::string(key) + "\" must be in " + range);
  return static_cast<int>(*v);
}

std::size_t player(const json::Object& r) {
  return static_cast<std::size_t>(integer(r, "player", 1, 1, static_cast<int>(core::kMaxPlayers)) - 1);
}

std::string respond(const std::string& id, bool ok, const Fields& fields) {
  Fields all;
  if (!id.empty()) all.emplace_back("id", id);
  all.emplace_back("ok", json::boolean(ok));
  all.insert(all.end(), fields.begin(), fields.end());
  return json::object(all);
}

}  // namespace

Dispatcher::Dispatcher(Session& session, std::string token)
    : session_(session), token_(std::move(token)), authed_(token_.empty()),
      commands_{{"hello", &Dispatcher::hello},     {"state", &Dispatcher::state},
                {"pause", &Dispatcher::pause},     {"resume", &Dispatcher::resume},
                {"step", &Dispatcher::step},       {"press", &Dispatcher::press},
                {"hold", &Dispatcher::hold},       {"release", &Dispatcher::release},
                {"axis", &Dispatcher::axis},       {"click", &Dispatcher::click},
                {"screenshot", &Dispatcher::screenshot}, {"logs", &Dispatcher::logs},
                {"quit", &Dispatcher::quit}} {}

std::string Dispatcher::greeting() const {
  return json::object({{"protocol", json::str(kProtocol)},
                       {"version", json::num(kProtocolVersion)},
                       {"engine", json::str(core::version())},
                       {"auth", json::boolean(!token_.empty())},
                       {"commands", json::str(command_names())}});
}

std::string Dispatcher::command_names() const {
  std::string out;
  for (const auto& [name, fn] : commands_) out += (out.empty() ? "" : " ") + name;
  return out;
}

std::string Dispatcher::handle(std::string_view line) {
  std::string id;
  try {
    const json::Object request = json::parse_object(line);
    if (const auto it = request.find("id"); it != request.end()) id = json::raw(it->second);
    return respond(id, true, run(request));
  } catch (const std::exception& e) {
    return respond(id, false, {{"error", json::str(e.what())}});
  }
}

Fields Dispatcher::run(const json::Object& request) {
  const std::string cmd = text(request, "cmd");
  if (!authed_ && cmd != "hello") {
    throw core::Error("not authenticated: send {\"cmd\":\"hello\",\"token\":\"...\"} first "
                      "(the token is in ALPHA_CONTROL_TOKEN of whoever started the engine)");
  }
  const auto it = commands_.find(cmd);
  if (it == commands_.end()) throw core::Error("unknown command \"" + cmd + "\"; commands: " + command_names());
  return (this->*(it->second))(request);
}

Fields Dispatcher::hello(const json::Object& r) {
  if (!token_.empty() && (!find(r, "token") || text(r, "token") != token_)) {
    throw core::Error("wrong or missing \"token\"");
  }
  authed_ = true;
  return {{"protocol", json::str(kProtocol)}, {"version", json::num(kProtocolVersion)}};
}

Fields Dispatcher::state(const json::Object&) { return state_fields(session_.state()); }

Fields Dispatcher::pause(const json::Object&) {
  session_.set_paused(true);
  return {};
}

Fields Dispatcher::resume(const json::Object&) {
  session_.set_paused(false);
  return {};
}

Fields Dispatcher::step(const json::Object& r) {
  session_.step(integer(r, "frames", 1, 1, kMaxStepFrames));
  const GameState s = session_.state();
  return {{"frame", json::num(static_cast<double>(s.frame))},
          {"scene", json::str(s.scene)},
          {"error", s.error.empty() ? "null" : json::str(s.error)}};
}

Fields Dispatcher::press(const json::Object& r) { return inject(r, Press::Tap); }
Fields Dispatcher::hold(const json::Object& r) { return inject(r, Press::Hold); }
Fields Dispatcher::release(const json::Object& r) { return inject(r, Press::Release); }

Fields Dispatcher::inject(const json::Object& r, Press press) {
  session_.action(text(r, "action"), press, player(r));
  return {};
}

Fields Dispatcher::axis(const json::Object& r) {
  const auto value = number(r, "value");
  if (value && (*value < -1.0 || *value > 1.0)) throw core::Error("\"value\" must be in -1..1 (null releases)");
  session_.axis(text(r, "axis"), player(r), value ? std::optional<float>(static_cast<float>(*value)) : std::nullopt);
  return {};
}

Fields Dispatcher::click(const json::Object& r) {
  const std::string target = text(r, "target");
  const auto hash = target.find('#');
  if (hash == std::string::npos) throw core::Error("\"target\" must be \"doc#id\", got \"" + target + "\"");
  session_.click(target.substr(0, hash), target.substr(hash + 1));
  return {};
}

Fields Dispatcher::screenshot(const json::Object&) {
  const Png png = session_.screenshot();
  return {{"png", json::str(base64(png.bytes))}, {"width", json::num(png.width)}, {"height", json::num(png.height)}};
}

Fields Dispatcher::logs(const json::Object& r) {
  return log_fields(session_.log(), static_cast<std::size_t>(integer(r, "since", 0, 0, INT_MAX)));
}

Fields Dispatcher::quit(const json::Object&) {
  session_.quit();
  return {};
}

}  // namespace alpha::mcp
