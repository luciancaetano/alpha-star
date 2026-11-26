#include "platform/net_ws.hpp"

#include <SDL3/SDL.h>

#if defined(_MSC_VER)  // wslay's callbacks return the POSIX ssize_t
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include <wslay/wslay.h>

#include <algorithm>
#include <random>
#include <vector>

#include "net/sha1.hpp"
#include "net/ws_handshake.hpp"

namespace alpha::platform {

namespace {

constexpr std::size_t kMaxResponseHead = 16384;
constexpr Uint64 kCloseWaitMs = 3000;  // a peer that never answers our close

void random_bytes(std::uint8_t* out, std::size_t n) {
  std::random_device rd;
  for (std::size_t i = 0; i < n; ++i) out[i] = static_cast<std::uint8_t>(rd());
}

class WsLink final : public net::Link {
 public:
  WsLink(const net::Address& a, std::unique_ptr<net::ByteStream> stream)
      : stream_(std::move(stream)), protocols_(a.protocols), max_(a.max_message) {
    std::string key(16, '\0');
    random_bytes(reinterpret_cast<std::uint8_t*>(key.data()), key.size());
    key_ = net::base64(key);
    stream_->write(net::ws_request(a, key_));
  }
  ~WsLink() override {
    if (ctx_) wslay_event_context_free(ctx_);
  }
  WsLink(const WsLink&) = delete;
  WsLink& operator=(const WsLink&) = delete;

  net::LinkStatus pump(std::deque<net::Message>& in) override {
    if (status_.state == net::State::closed) return status_;
    std::string bytes;
    const net::StreamState s = stream_->pump(bytes);
    if (!ctx_) handshake(bytes);
    if (ctx_ && status_.state != net::State::closed) frames(bytes, in);
    if (s == net::StreamState::closed && status_.state != net::State::closed) lost();
    if (status_.state == net::State::closing && SDL_GetTicks() > close_deadline_) finish(status_.close_code.value_or(1006), "");
    return status_;
  }
  void write(std::deque<net::Message>& out) override {
    if (!ctx_) return;
    for (; !out.empty(); out.pop_front()) {
      const std::string& m = out.front().data;
      const wslay_event_msg msg{static_cast<std::uint8_t>(out.front().text ? WSLAY_TEXT_FRAME : WSLAY_BINARY_FRAME),
                                reinterpret_cast<const std::uint8_t*>(m.data()), m.size()};
      wslay_event_queue_msg(ctx_, &msg);
    }
    wslay_event_send(ctx_);
  }
  void close(int code, const std::string& reason) override {
    if (!ctx_) return finish(std::nullopt, "");
    wslay_event_queue_close(ctx_, static_cast<std::uint16_t>(code), reinterpret_cast<const std::uint8_t*>(reason.data()), reason.size());
    wslay_event_send(ctx_);
    status_ = {net::State::closing, "", code};
    close_deadline_ = SDL_GetTicks() + kCloseWaitMs;
  }

 private:
  void handshake(std::string& bytes) {
    head_ += bytes;
    bytes.clear();
    const auto end = head_.find("\r\n\r\n");
    if (end == std::string::npos) {
      if (head_.size() > kMaxResponseHead) finish(std::nullopt, "ws handshake failed: response headers over 16 KiB");
      return;
    }
    if (std::string e = net::ws_check_response(std::string_view(head_).substr(0, end + 2), key_, protocols_); !e.empty()) {
      return finish(std::nullopt, e);
    }
    bytes = head_.substr(end + 4);  // frames the server sent right after its answer
    start();
  }
  void start() {
    static constexpr wslay_event_callbacks kCallbacks{recv_cb, send_cb, genmask_cb, nullptr, nullptr, nullptr, msg_cb};
    wslay_event_context_client_init(&ctx_, &kCallbacks, this);
    wslay_event_config_set_max_recv_msg_length(ctx_, max_);
    status_.state = net::State::open;
  }
  void frames(const std::string& bytes, std::deque<net::Message>& in) {
    rx_ += bytes;
    in_ = &in;
    const int r = wslay_event_recv(ctx_);
    in_ = nullptr;
    wslay_event_send(ctx_);
    if (r != 0) return finish(1002, "ws protocol error from the server");
    if (wslay_event_get_status_code_sent(ctx_) == WSLAY_CODE_MESSAGE_TOO_BIG) {
      return finish(1009, "peer sent a message over maxMessageBytes " + net::size_text(max_));
    }
    if (wslay_event_get_close_received(ctx_)) finish(wslay_event_get_status_code_received(ctx_), "");
  }
  void lost() {
    const bool clean = status_.state == net::State::closing;
    finish(status_.close_code, clean ? "" : stream_->error().empty() ? "connection closed without a close frame" : stream_->error());
  }
  void finish(std::optional<int> code, std::string error) { status_ = {net::State::closed, std::move(error), code}; }

  static ssize_t recv_cb(wslay_event_context_ptr ctx, std::uint8_t* buf, std::size_t len, int, void* user) {
    auto& self = *static_cast<WsLink*>(user);
    if (self.rx_.empty()) {
      wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
      return -1;
    }
    const std::size_t n = std::min(len, self.rx_.size());
    std::copy_n(self.rx_.data(), n, reinterpret_cast<char*>(buf));
    self.rx_.erase(0, n);
    return static_cast<ssize_t>(n);
  }
  static ssize_t send_cb(wslay_event_context_ptr, const std::uint8_t* data, std::size_t len, int, void* user) {
    static_cast<WsLink*>(user)->stream_->write(std::string_view(reinterpret_cast<const char*>(data), len));
    return static_cast<ssize_t>(len);
  }
  static int genmask_cb(wslay_event_context_ptr, std::uint8_t* buf, std::size_t len, void*) {
    random_bytes(buf, len);
    return 0;
  }
  static void msg_cb(wslay_event_context_ptr, const wslay_event_on_msg_recv_arg* arg, void* user) {
    auto& self = *static_cast<WsLink*>(user);
    if (arg->opcode != WSLAY_TEXT_FRAME && arg->opcode != WSLAY_BINARY_FRAME) return;
    self.in_->push_back({std::string(reinterpret_cast<const char*>(arg->msg), arg->msg_length), arg->opcode == WSLAY_TEXT_FRAME});
  }

  std::unique_ptr<net::ByteStream> stream_;
  std::vector<std::string> protocols_;
  std::size_t max_;
  std::string key_, head_, rx_;
  wslay_event_context_ptr ctx_ = nullptr;
  std::deque<net::Message>* in_ = nullptr;
  net::LinkStatus status_;
  Uint64 close_deadline_ = 0;
};

}  // namespace

std::unique_ptr<net::Link> ws_link(const net::Address& address, std::unique_ptr<net::ByteStream> stream) {
  return std::make_unique<WsLink>(address, std::move(stream));
}

}  // namespace alpha::platform
