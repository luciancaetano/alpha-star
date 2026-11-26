#include "net/socket.hpp"

#include "core/error.hpp"
#include "core/limits.hpp"

namespace alpha::net {

namespace {

std::size_t bytes_of(const std::deque<Message>& q) {
  std::size_t n = 0;
  for (const Message& m : q) n += m.data.size();
  return n;
}

}  // namespace

std::string endpoint_of(const Address& a) {
  if (a.kind != Kind::ws) return a.host + ":" + std::to_string(a.port);
  return std::string(a.tls ? "wss://" : "ws://") + a.host + ":" + std::to_string(a.port) + a.path;
}

Socket::Socket(const Address& address, std::unique_ptr<Link> link)
    : kind_(address.kind), framing_(address.framing), endpoint_(endpoint_of(address)), max_message_(address.max_message), link_(std::move(link)) {
  if (kind_ == Kind::udp) live_.state = seen_.state = State::open;
}

Socket::~Socket() {
  if (link_ && live_.state != State::closed) link_->close(1001, "going away");
}

void Socket::pump() {
  if (!link_ || live_.state == State::closed) return;
  const std::size_t before = arrived_.size();
  settle(link_->pump(arrived_));
  for (std::size_t i = before; i < arrived_.size(); ++i) arrived_bytes_ += arrived_[i].data.size();
  if (arrived_bytes_ + inbox_bytes_ > core::kMaxNetQueueBytes) {
    fail("inbound queue full (" + size_text(core::kMaxNetQueueBytes) + "): call receive() every step; the game may have been paused");
  }
}

void Socket::settle(LinkStatus status) {
  if (live_.state == State::closed) return;
  if (seen_.state == State::closing && status.state != State::closed) status.state = State::closing;
  live_ = std::move(status);
  if (live_.state == State::closed) outbox_.clear(), outbox_bytes_ = 0;
}

void Socket::fail(std::string error) {
  if (link_) link_->close(1009, "message too big");
  live_ = {State::closed, std::move(error), std::nullopt};
  outbox_.clear();
  outbox_bytes_ = 0;
}

std::size_t Socket::deliver(std::uint64_t step) {
  const std::size_t count = arrived_.size();
  for (Message& m : arrived_) {
    m.step = step;
    received_ += m.data.size();
    inbox_.push_back(std::move(m));
  }
  inbox_bytes_ += arrived_bytes_;
  arrived_.clear();
  arrived_bytes_ = 0;
  seen_ = live_;
  return count;
}

void Socket::replay(std::uint64_t step, const LinkStatus& status, const std::vector<Message>& messages) {
  for (const Message& m : messages) {
    received_ += m.data.size();
    inbox_bytes_ += m.data.size();
    inbox_.push_back({m.data, m.text, step});
  }
  if (seen_.state != State::closed) live_ = seen_ = status;
}

std::vector<Message> Socket::newest(std::size_t n) const {
  return {inbox_.end() - static_cast<std::ptrdiff_t>(std::min(n, inbox_.size())), inbox_.end()};
}

void Socket::flush() {
  if (!link_ || live_.state != State::open || outbox_.empty()) return;
  link_->write(outbox_);
  const std::size_t left = bytes_of(outbox_);
  sent_ += outbox_bytes_ - left;
  outbox_bytes_ = left;
}

bool Socket::send(Message message) {
  if (message.data.size() > max_message_) {
    throw core::Error("socket.send: message of " + std::to_string(message.data.size()) + " bytes exceeds maxMessageBytes " +
                      std::to_string(max_message_));
  }
  if (kind_ == Kind::tcp) check_framable(framing_, message.data);
  // Only WebSocket keeps text and bytes apart on the wire; tcp "lines" is always text.
  if (kind_ != Kind::ws) message.text = kind_ == Kind::tcp && framing_ == Framing::lines;
  if (seen_.state == State::closing || seen_.state == State::closed) return false;
  if (outbox_bytes_ + message.data.size() > core::kMaxNetQueueBytes) return false;
  outbox_bytes_ += message.data.size();
  outbox_.push_back(std::move(message));
  return true;
}

std::vector<Message> Socket::receive() {
  std::vector<Message> out(std::make_move_iterator(inbox_.begin()), std::make_move_iterator(inbox_.end()));
  inbox_.clear();
  inbox_bytes_ = 0;
  return out;
}

void Socket::close(int code, const std::string& reason) {
  if (seen_.state == State::closing || seen_.state == State::closed) return;
  flush();  // what was sent before close() still goes out
  const bool was_open = live_.state == State::open;
  if (link_) link_->close(code, reason);
  seen_ = {was_open && kind_ != Kind::udp ? State::closing : State::closed, "", std::nullopt};
  if (seen_.state == State::closed) live_ = seen_;
  else live_.state = State::closing;
  outbox_.clear();
  outbox_bytes_ = 0;
}

Stats Socket::stats() const { return {sent_, received_, outbox_bytes_, inbox_bytes_}; }

}  // namespace alpha::net
