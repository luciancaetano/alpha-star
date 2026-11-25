#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mcp/json.hpp"
#include "mcp/session.hpp"

namespace alpha::mcp {

// Agent control protocol ("alpha-control", line-delimited JSON over a local
// socket). Every request is one flat JSON object on one line:
//   {"id": 1, "cmd": "press", "action": "flap", "player": 1}
// and gets exactly one response line echoing the id:
//   {"id": 1, "ok": true, ...}  or  {"id": 1, "ok": false, "error": "..."}
inline constexpr std::string_view kProtocol = "alpha-control";
inline constexpr int kProtocolVersion = 1;
inline constexpr int kMaxStepFrames = 36000;  // 10 minutes of game time per request

using Fields = std::vector<std::pair<std::string, std::string>>;

// Turns request lines into Session calls and response lines. With a token,
// a connection must send {"cmd":"hello","token":...} before anything else.
class Dispatcher {
 public:
  Dispatcher(Session& session, std::string token);
  // First line sent on every new connection (protocol, version, commands).
  std::string greeting() const;
  // A new connection starts unauthenticated.
  void reset() { authed_ = token_.empty(); }
  // One request line -> one response line (no newline).
  std::string handle(std::string_view line);

 private:
  using Command = Fields (Dispatcher::*)(const json::Object&);
  Fields run(const json::Object& request);
  Fields hello(const json::Object& r);
  Fields state(const json::Object& r);
  Fields pause(const json::Object& r);
  Fields resume(const json::Object& r);
  Fields step(const json::Object& r);
  Fields press(const json::Object& r);
  Fields hold(const json::Object& r);
  Fields release(const json::Object& r);
  Fields inject(const json::Object& r, Press press);
  Fields axis(const json::Object& r);
  Fields click(const json::Object& r);
  Fields screenshot(const json::Object& r);
  Fields logs(const json::Object& r);
  Fields quit(const json::Object& r);
  Fields reload(const json::Object& r);
  std::string command_names() const;

  Session& session_;
  std::string token_;
  bool authed_;
  std::map<std::string, Command, std::less<>> commands_;
};

}  // namespace alpha::mcp
