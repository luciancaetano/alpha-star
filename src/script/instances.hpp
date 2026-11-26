#pragma once

#include <quickjs.h>

#include <cstdint>
#include <map>
#include <memory>

#include "core/log_ring.hpp"
#include "net/link.hpp"
#if ALPHA_DEBUG_TOOLS
#include "net/fake_net.hpp"
#endif
#include "script/script_state.hpp"

namespace alpha::script {

// Every game instance living in one JS context: the host's own game (id 0)
// and one per simulate() run, each with its own world, timers, random and
// assets. Natives act on the active one; the runtime selects it around every
// call into an instance, so instances never see each other's state.
class Instances {
 public:
  static constexpr std::uint32_t kMain = 0;

  // log: console.* lines and pausing errors of every instance (owned by the host).
  Instances(std::function<std::string(const std::string&)> save_path_for, core::LogRing& log);
  ScriptState& active() { return *active_; }
  ScriptState& main() { return *states_.at(kMain); }
  // A fresh instance whose assets resolve like the main one's; it never saves to disk.
  std::uint32_t create();
  // Makes `id` active; returns the id that was active. Throws Error if unknown.
  std::uint32_t use(std::uint32_t id);
  // Throws Error for the main or the active instance.
  void destroy(std::uint32_t id);
  // newest last.
  core::LogRing& log() { return log_; }
  const core::LogRing& log() const { return log_; }
  // What alpha:net sockets connect through (real network, or the test fake).
  const net::LinkFactory& net_links() const { return links_; }
#if ALPHA_DEBUG_TOOLS
  // Test runs: a fresh in-memory network; every socket opened after this reaches it.
  void reset_fake_net();
  // Counts reset_fake_net calls: alpha/test forgets servers of an older network.
  std::uint32_t fake_generation() const { return fake_generation_; }
  // The current fake (alpha/test `netFake`); throws Error outside test runs.
  net::FakeNet& fake_net();
  // --net-record: the main instance's hub keeps its deliveries (tape_text at exit).
  void record_net();
  // --net-replay: sockets reach no network; the main instance's deliveries come from `tape`.
  // Throws core::Error when it does not parse.
  void replay_net(const std::string& tape);
  std::string tape_text() const { return tape_ ? tape_->text() : ""; }
  // Gives a new hub of `state` the tape (the main instance only).
  void attach_tape(ScriptState& state);
#endif

 private:
  ScriptState& at(std::uint32_t id);

  std::map<std::uint32_t, std::unique_ptr<ScriptState>> states_;
  ScriptState* active_;
  std::uint32_t active_id_ = kMain;
  std::uint32_t next_id_ = kMain + 1;
  core::LogRing& log_;
  net::LinkFactory links_;
#if ALPHA_DEBUG_TOOLS
  std::unique_ptr<net::FakeNet> fake_;
  std::uint32_t fake_generation_ = 0;
  std::shared_ptr<net::Tape> tape_;
  bool replay_ = false;
#endif
};

inline Instances& instances_of(JSContext* ctx) { return *static_cast<Instances*>(JS_GetContextOpaque(ctx)); }
inline ScriptState& state_of(JSContext* ctx) { return instances_of(ctx).active(); }

}  // namespace alpha::script
