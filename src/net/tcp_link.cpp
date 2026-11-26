#include "net/tcp_link.hpp"

namespace alpha::net {

TcpLink::TcpLink(std::unique_ptr<ByteStream> stream, Framing framing, std::size_t max_message)
    : stream_(std::move(stream)), framing_(framing), deframer_(framing, max_message) {}

LinkStatus TcpLink::pump(std::deque<Message>& in) {
  if (closed_) return {State::closed, error_, std::nullopt};
  std::string bytes;
  const StreamState state = stream_->pump(bytes);
  error_ = deframer_.push(bytes, in);
  if (error_.empty() && state != StreamState::closed) {
    return {state == StreamState::open ? State::open : State::connecting, "", std::nullopt};
  }
  if (error_.empty()) error_ = stream_->error();
  closed_ = true;
  stream_.reset();  // closes the connection
  return {State::closed, error_, std::nullopt};
}

void TcpLink::write(std::deque<Message>& out) {
  for (; !out.empty() && stream_; out.pop_front()) stream_->write(frame(framing_, out.front().data));
}

void TcpLink::close(int, const std::string&) {
  closed_ = true;
  stream_.reset();
}

}  // namespace alpha::net
