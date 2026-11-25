#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <string>

#include "mcp/control.hpp"
#include "mcp/socket.hpp"
#include "mcp_fixture.hpp"

using alpha::mcp::Control;
using alpha::mcp::ControlConfig;
using alpha::mcp::Socket;

namespace {

struct Rig {
  alpha::test::McpGame game;
  Control control;
  explicit Rig(ControlConfig config = {true, 0, false, ""})
      : control(config, game.host, game.renderer, [] { return alpha::core::InputFrame{}; }, alpha::test::kStepDt) {}
  // Polls the control channel until `client` has a full line; returns it (without the newline).
  std::string line(const Socket& client, std::string& buffer) {
    for (int i = 0; i < 20000 && buffer.find('\n') == std::string::npos; ++i) {
      control.poll(16.0);
      client.receive(buffer);
    }
    const auto nl = buffer.find('\n');
    std::string out = buffer.substr(0, nl == std::string::npos ? buffer.size() : nl);
    buffer.erase(0, nl == std::string::npos ? buffer.size() : nl + 1);
    return out;
  }
};

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("control: a client gets the greeting, then one response per request line") {
  Rig rig;
  REQUIRE(rig.control.port() > 0);
  const Socket client = Socket::connect(rig.control.port());
  std::string buf;
  CHECK(has(rig.line(client, buf), R"("protocol":"alpha-control")"));
  client.send("{\"id\":1,\"cmd\":\"state\"}\r\n\n{\"id\":2,\"cmd\":\"pause\"}\n");
  CHECK(has(rig.line(client, buf), R"("scene":"title")"));
  CHECK(has(rig.line(client, buf), R"({"id":2,"ok":true})"));
  CHECK(rig.control.paused());
  CHECK(rig.control.steps(3) == 0);
}

TEST_CASE("control: one client at a time; a closed client frees the slot") {
  Rig rig;
  std::string buf;
  {
    const Socket first = Socket::connect(rig.control.port());
    rig.line(first, buf);
    const Socket second = Socket::connect(rig.control.port());
    std::string other;
    CHECK(has(rig.line(second, other), "busy"));
  }
  buf.clear();
  const Socket third = Socket::connect(rig.control.port());
  CHECK(has(rig.line(third, buf), "alpha-control"));
}

TEST_CASE("control: a token locks everything but hello") {
  Rig rig({true, 0, false, "tok"});
  const Socket client = Socket::connect(rig.control.port());
  std::string buf;
  CHECK(has(rig.line(client, buf), R"("auth":true)"));
  client.send("{\"cmd\":\"state\"}\n");
  CHECK(has(rig.line(client, buf), "not authenticated"));
  client.send("{\"cmd\":\"hello\",\"token\":\"tok\"}\n{\"cmd\":\"state\"}\n");
  CHECK(has(rig.line(client, buf), R"("ok":true)"));
  CHECK(has(rig.line(client, buf), R"("scene":"title")"));
}

TEST_CASE("control: quit ends the loop; an endless line closes the client") {
  Rig rig;
  const Socket client = Socket::connect(rig.control.port());
  std::string buf;
  rig.line(client, buf);
  // Flood without a newline, in chunks the server drains between polls; past
  // the line limit it answers "too long" and closes, freeing the slot.
  const std::string chunk(16384, 'x');
  for (int i = 0; i < 80 && client.send(chunk); ++i) rig.control.poll(16.0);
  const Socket again = Socket::connect(rig.control.port());
  buf.clear();
  rig.line(again, buf);
  again.send("{\"cmd\":\"quit\"}\n");
  CHECK(has(rig.line(again, buf), R"("ok":true)"));
  CHECK_FALSE(rig.control.poll(16.0));
}

TEST_CASE("control: without --control nothing listens and the game just runs") {
  Rig rig({false, 0, false, ""});
  CHECK(rig.control.port() == 0);
  CHECK(rig.control.poll(16.0));
  CHECK(rig.control.steps(2) == 2);
  rig.control.step();
  CHECK_FALSE(rig.control.paused());
}

TEST_CASE("control: hidden windows are paced to about 60 fps") {
  Rig rig({false, 0, true, ""});
  rig.control.poll(16.0);
  const Uint64 start = SDL_GetTicksNS();
  rig.control.poll(16.0);
  rig.control.poll(16.0);
  CHECK(SDL_GetTicksNS() - start >= 25'000'000U);
}

TEST_CASE("control: config comes from --control, --hidden and ALPHA_CONTROL_TOKEN") {
  alpha::core::CliOptions o;
  CHECK_FALSE(alpha::mcp::control_config(o).listen);
  o.control_port = 5000;
  o.hidden = true;
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_CONTROL_TOKEN", "abc", true);
  const ControlConfig c = alpha::mcp::control_config(o);
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_CONTROL_TOKEN");
  CHECK(c.listen);
  CHECK(c.port == 5000);
  CHECK(c.hidden);
  CHECK(c.token == "abc");
  CHECK(alpha::mcp::control_config(o).token.empty());
}
