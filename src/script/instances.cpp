#include "script/instances.hpp"

#include <string>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "platform/net_links.hpp"

namespace alpha::script {

Instances::Instances(std::function<std::string(const std::string&)> save_path_for, core::LogRing& log)
    : log_(log), links_(platform::real_links()) {
  auto state = std::make_unique<ScriptState>();
  state->save_path_for = std::move(save_path_for);
  active_ = state.get();
  states_.emplace(kMain, std::move(state));
}

ScriptState& Instances::at(std::uint32_t id) {
  const auto it = states_.find(id);
  if (it == states_.end()) throw core::Error("no game instance #" + std::to_string(id));
  return *it->second;
}

std::uint32_t Instances::create() {
  if (states_.size() >= core::kMaxInstances) {
#if ALPHA_DEBUG_TOOLS
    throw core::Error("at most " + std::to_string(core::kMaxInstances) + " game instances (simulate runs) at once");
#else  // shipping strings never name debug tools (tools/check_shipping.py)
    throw core::Error("at most " + std::to_string(core::kMaxInstances) + " game instances at once");
#endif
  }
  auto state = std::make_unique<ScriptState>();
  state->asset_dir = main().asset_dir;
  state->save_path_for = [](const std::string&) { return std::string(); };
  states_.emplace(next_id_, std::move(state));
  return next_id_++;
}

std::uint32_t Instances::use(std::uint32_t id) {
  active_ = &at(id);
  const std::uint32_t previous = active_id_;
  active_id_ = id;
  return previous;
}

void Instances::destroy(std::uint32_t id) {
  if (id == kMain || id == active_id_) throw core::Error("cannot destroy the main or the active game instance");
  states_.erase(id);
}

#if ALPHA_DEBUG_TOOLS
void Instances::reset_fake_net() {
  fake_ = std::make_unique<net::FakeNet>();
  ++fake_generation_;
  links_ = [this](const net::Address& a) { return fake_->factory()(a); };
}

namespace {

// --net-replay: a socket that never reaches anything; the tape says what it saw.
class SilentLink final : public net::Link {
 public:
  net::LinkStatus pump(std::deque<net::Message>&) override { return {}; }
  void write(std::deque<net::Message>& out) override { out.clear(); }
  void close(int, const std::string&) override {}
};

}  // namespace

void Instances::record_net() {
  tape_ = std::make_shared<net::Tape>();
  replay_ = false;
}

void Instances::replay_net(const std::string& tape) {
  tape_ = std::make_shared<net::Tape>(net::Tape::parse(tape));
  replay_ = true;
  links_ = [](const net::Address&) { return std::make_unique<SilentLink>(); };
}

void Instances::attach_tape(ScriptState& state) {
  if (tape_ && &state == &main() && state.net) state.net->use_tape(tape_, replay_);
}

net::FakeNet& Instances::fake_net() {
  if (!fake_) throw core::Error("netFake works only in test runs (alpha test)");
  return *fake_;
}
#endif

}  // namespace alpha::script
