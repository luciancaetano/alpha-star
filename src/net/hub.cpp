#include "net/hub.hpp"

#include <algorithm>
#include <string>

#include "core/error.hpp"
#include "core/limits.hpp"

namespace alpha::net {

Hub::Hub(LinkFactory make) : make_(std::move(make)) {}

Hub::~Hub() { close_all(); }

std::uint32_t Hub::open(const Address& address) {
  std::erase_if(sockets_, [](const auto& kv) { return kv.second->spent(); });
  const auto live = std::count_if(sockets_.begin(), sockets_.end(),
                                  [](const auto& kv) { return kv.second->state() != State::closed; });
  if (static_cast<std::size_t>(live) >= core::kMaxNetSockets) {
    throw core::Error(std::string("net.") + kind_name(address.kind) + ": too many open sockets (" +
                      std::to_string(core::kMaxNetSockets) + ", limit kMaxNetSockets); close unused ones");
  }
  const std::uint32_t id = next_id_++;
  sockets_.emplace(id, std::make_unique<Socket>(address, make_(address)));
  return id;
}

Socket* Hub::find(std::uint32_t id) {
  const auto it = sockets_.find(id);
  return it == sockets_.end() ? nullptr : it->second.get();
}

void Hub::pump() {
  for (auto& [id, s] : sockets_) s->pump();
}

#if ALPHA_DEBUG_TOOLS
void Hub::deliver() {
  ++step_;
  if (!replay_) pump();
  for (auto& [id, s] : sockets_) {
    if (replay_) {
      if (const Tape::Event* e = tape_->at(id, step_)) s->replay(step_, e->status, e->messages);
      continue;
    }
    const std::size_t n = s->deliver(step_);
    if (tape_) tape_->record(id, step_, s->seen(), s->newest(n));
  }
}

void Hub::use_tape(std::shared_ptr<Tape> tape, bool replay) {
  tape_ = std::move(tape);
  replay_ = replay && tape_;
}
#else
void Hub::deliver() {
  ++step_;
  pump();
  for (auto& [id, s] : sockets_) s->deliver(step_);
}
#endif

void Hub::flush() {
  for (auto& [id, s] : sockets_) s->flush();
}

void Hub::close_all() {
  for (auto& [id, s] : sockets_) s->close(1001, "going away");
}

}  // namespace alpha::net
