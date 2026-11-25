#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "core/error.hpp"
#include "mcp/dispatch.hpp"
#include "mcp/json.hpp"

using namespace alpha::mcp;
using alpha::core::Error;

namespace {

// Records what the dispatcher asked for; throws like the real session would.
struct FakeSession final : Session {
  bool paused = false;
  std::vector<std::string> calls;
  alpha::core::LogRing ring{3};

  GameState state() override {
    GameState s;
    s.scene = "title";
    s.paused = paused;
    s.frame = 42;
    s.fps = 60;
    s.entities.push_back({7, {1.5f, 2}, 90, 1, {"bird"}, alpha::core::Vec2{0, -3}, "", "", 1.0f, std::nullopt, 0, R"({"hp":2})"});
    s.joints = 3;
    s.locale = "pt";
    s.music = "theme";
    s.app_paused = true;
    s.ui["hud"] = {true, {{"score", "3"}}};
    return s;
  }
  void set_paused(bool p) override { paused = p; }
  void step(int frames) override {
    if (!paused) throw Error("step needs a paused game");
    calls.push_back("step " + std::to_string(frames));
  }
  void action(std::string_view name, Press press, std::size_t player) override {
    if (name != "flap") throw Error("unknown action \"" + std::string(name) + "\"; actions: flap");
    calls.push_back("action " + std::string(name) + " " + std::to_string(static_cast<int>(press)) + " " +
                    std::to_string(player));
  }
  void axis(std::string_view name, std::size_t player, std::optional<float> v) override {
    calls.push_back("axis " + std::string(name) + " " + std::to_string(player) + " " +
                    (v ? std::to_string(*v) : "release"));
  }
  void click(std::string_view doc, std::string_view id) override {
    calls.push_back("click " + std::string(doc) + " " + std::string(id));
  }
  Png screenshot() override { return {"foo", 4, 2}; }
  const alpha::core::LogRing& log() const override { return ring; }
  void quit() override { calls.push_back("quit"); }
  void reload() override {
    if (fail_reload) throw Error("reload failed; the previous game keeps running: SyntaxError");
    calls.push_back("reload");
  }
  bool fail_reload = false;
};

struct Rig {
  FakeSession session;
  Dispatcher d{session, ""};
  json::Object send(const std::string& line) { return json::parse_object(d.handle(line)); }
};

bool ok(const json::Object& r) { return std::get<bool>(r.at("ok")); }
std::string error(const json::Object& r) { return std::get<std::string>(r.at("error")); }

}  // namespace

TEST_CASE("control: the greeting names the protocol, its version and the commands") {
  Rig rig;
  const auto g = json::parse_object(rig.d.greeting());
  CHECK(std::get<std::string>(g.at("protocol")) == "alpha-control");
  CHECK(std::get<double>(g.at("version")) == 1);
  CHECK_FALSE(std::get<bool>(g.at("auth")));
  CHECK(std::get<std::string>(g.at("commands")).find("screenshot") != std::string::npos);
}

TEST_CASE("control: responses echo the id; errors explain and list what exists") {
  Rig rig;
  const auto r = rig.send(R"({"id":"a1","cmd":"dance"})");
  CHECK(std::get<std::string>(r.at("id")) == "a1");
  CHECK_FALSE(ok(r));
  CHECK(error(r).find("unknown command \"dance\"") != std::string::npos);
  CHECK(error(r).find("state") != std::string::npos);
  CHECK(error(rig.send(R"({"id":1})")).find("\"cmd\"") != std::string::npos);
  CHECK(error(rig.send("not json")).find("bad JSON") != std::string::npos);
  CHECK(std::get<double>(rig.send(R"({"id":9,"cmd":"pause"})").at("id")) == 9);
}

TEST_CASE("control: state reports scene, pause, entities and UI") {
  Rig rig;
  const std::string line = rig.d.handle(R"({"cmd":"state"})");
  CHECK(line.find(R"("scene":"title")") != std::string::npos);
  CHECK(line.find(R"("error":null)") != std::string::npos);
  CHECK(line.find(R"("frame":42)") != std::string::npos);
  CHECK(line.find(R"("joints":3)") != std::string::npos);
  CHECK(line.find(R"("locale":"pt","music":"theme","appPaused":true)") != std::string::npos);
  CHECK(line.find(R"({"id":7,"tags":["bird"],"pos":[1.5,2],"rotation":90,"z":1,"velocity":[0,-3],"animation":null,"layer":null,"alpha":1,"text":null,"particles":0,"data":{"hp":2}})") !=
        std::string::npos);
  CHECK(line.find(R"("ui":{"hud":{"shown":true,"texts":{"score":"3"}}})") != std::string::npos);
}

TEST_CASE("control: pause, step, resume") {
  Rig rig;
  CHECK(error(rig.send(R"({"cmd":"step"})")).find("paused") != std::string::npos);
  CHECK(ok(rig.send(R"({"cmd":"pause"})")));
  const auto r = rig.send(R"({"cmd":"step","frames":30})");
  CHECK(ok(r));
  CHECK(std::get<double>(r.at("frame")) == 42);
  CHECK(std::get<std::string>(r.at("scene")) == "title");
  CHECK(ok(rig.send(R"({"cmd":"step"})")));
  CHECK(rig.session.calls == std::vector<std::string>{"step 30", "step 1"});
  CHECK(error(rig.send(R"({"cmd":"step","frames":0})")).find("frames") != std::string::npos);
  CHECK(error(rig.send(R"({"cmd":"step","frames":1.5})")).find("integer") != std::string::npos);
  CHECK(error(rig.send(R"({"cmd":"step","frames":"2"})")).find("number") != std::string::npos);
  CHECK(ok(rig.send(R"({"cmd":"resume"})")));
  CHECK_FALSE(rig.session.paused);
}

TEST_CASE("control: actions and axes reach the session per player") {
  Rig rig;
  CHECK(ok(rig.send(R"({"cmd":"press","action":"flap"})")));
  CHECK(ok(rig.send(R"({"cmd":"hold","action":"flap","player":2})")));
  CHECK(ok(rig.send(R"({"cmd":"release","action":"flap","player":2})")));
  CHECK(error(rig.send(R"({"cmd":"press","action":"jump"})")).find("actions: flap") != std::string::npos);
  CHECK(error(rig.send(R"({"cmd":"press","action":"flap","player":5})")).find("1..4") != std::string::npos);
  CHECK(error(rig.send(R"({"cmd":"press"})")).find("\"action\"") != std::string::npos);
  CHECK(ok(rig.send(R"({"cmd":"axis","axis":"moveX","value":-0.5})")));
  CHECK(ok(rig.send(R"({"cmd":"axis","axis":"moveX","value":null,"player":3})")));
  CHECK(error(rig.send(R"({"cmd":"axis","axis":"moveX","value":2})")).find("-1..1") != std::string::npos);
  CHECK(rig.session.calls == std::vector<std::string>{"action flap 0 0", "action flap 1 1", "action flap 2 1",
                                                      "axis moveX 0 -0.500000", "axis moveX 2 release"});
}

TEST_CASE("control: click takes doc#id") {
  Rig rig;
  CHECK(ok(rig.send(R"({"cmd":"click","target":"over#again"})")));
  CHECK(error(rig.send(R"({"cmd":"click","target":"again"})")).find("doc#id") != std::string::npos);
  CHECK(rig.session.calls == std::vector<std::string>{"click over again"});
}

TEST_CASE("control: screenshot is base64 PNG with its size") {
  Rig rig;
  const auto r = rig.send(R"({"cmd":"screenshot"})");
  CHECK(std::get<std::string>(r.at("png")) == "Zm9v");
  CHECK(std::get<double>(r.at("width")) == 4);
  CHECK(std::get<double>(r.at("height")) == 2);
}

TEST_CASE("control: logs since a cursor, reporting dropped lines") {
  Rig rig;
  for (const char* t : {"a", "b", "c", "d"}) rig.session.ring.push(alpha::core::LogLevel::Info, t);
  rig.session.ring.push(alpha::core::LogLevel::Error, "boom");
  const std::string all = rig.d.handle(R"({"cmd":"logs"})");
  CHECK(all.find(R"("lines":[{"n":2,"level":"log","text":"c"},{"n":3,"level":"log","text":"d"},)"
                 R"({"n":4,"level":"error","text":"boom"}])") != std::string::npos);
  CHECK(all.find(R"("next":5)") != std::string::npos);
  CHECK(all.find(R"("dropped":2)") != std::string::npos);
  const std::string tail = rig.d.handle(R"({"cmd":"logs","since":4})");
  CHECK(tail.find(R"("lines":[{"n":4,"level":"error","text":"boom"}])") != std::string::npos);
  CHECK(tail.find(R"("dropped":0)") != std::string::npos);
  CHECK(rig.d.handle(R"({"cmd":"logs","since":9})").find(R"("lines":[])") != std::string::npos);
}

TEST_CASE("control: quit is acknowledged") {
  Rig rig;
  CHECK(ok(rig.send(R"({"cmd":"quit"})")));
  CHECK(rig.session.calls == std::vector<std::string>{"quit"});
}

TEST_CASE("control: with a token, nothing but hello works until it matches") {
  FakeSession session;
  Dispatcher d{session, "s3cret"};
  CHECK(std::get<bool>(json::parse_object(d.greeting()).at("auth")));
  const auto locked = json::parse_object(d.handle(R"({"cmd":"state"})"));
  CHECK(error(locked).find("hello") != std::string::npos);
  CHECK(error(json::parse_object(d.handle(R"({"cmd":"hello","token":"nope"})"))).find("token") != std::string::npos);
  CHECK(ok(json::parse_object(d.handle(R"({"cmd":"hello","token":"s3cret"})"))));
  CHECK(d.handle(R"({"cmd":"state"})").find(R"("ok":true)") != std::string::npos);
  d.reset();
  CHECK_FALSE(ok(json::parse_object(d.handle(R"({"cmd":"state"})"))));
}

TEST_CASE("control: reload answers with the scene, or the error while the old game runs on") {
  Rig rig;
  const auto r = rig.send(R"({"cmd":"reload"})");
  CHECK(ok(r));
  CHECK(std::get<std::string>(r.at("scene")) == "title");
  CHECK(std::get<double>(r.at("frame")) == 42);
  CHECK(rig.session.calls == std::vector<std::string>{"reload"});
  rig.session.fail_reload = true;
  CHECK(error(rig.send(R"({"cmd":"reload"})")).find("previous game keeps running") != std::string::npos);
}
