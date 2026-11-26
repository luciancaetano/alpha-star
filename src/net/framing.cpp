#include "net/framing.hpp"

#include <cstdint>

#include "core/error.hpp"

namespace alpha::net {

namespace {

std::size_t head_size(Framing framing) { return framing == Framing::u16 ? 2 : 4; }

std::string prefixed_frame(std::string_view payload, std::size_t head) {
  std::string out(head, '\0');
  for (std::size_t i = 0; i < head; ++i) {
    out[head - 1 - i] = static_cast<char>((payload.size() >> (8 * i)) & 0xFF);
  }
  return out.append(payload);
}

}  // namespace

std::optional<Framing> parse_framing(std::string_view name) {
  if (name == "lines") return Framing::lines;
  if (name == "u16") return Framing::u16;
  if (name == "u32") return Framing::u32;
  if (name == "raw") return Framing::raw;
  return std::nullopt;
}

std::size_t framing_cap(Framing framing) {
  return framing == Framing::u16 ? std::size_t{0xFFFF} : std::size_t{0xFFFFFFFF};
}

void check_framable(Framing framing, std::string_view payload) {
  if (payload.size() > framing_cap(framing)) {
    throw core::Error("socket.send: message of " + std::to_string(payload.size()) + " bytes does not fit u16 framing (max 65535)");
  }
  if (framing == Framing::lines && payload.find('\n') != std::string_view::npos) {
    throw core::Error("socket.send: a \"lines\" message cannot contain \\n");
  }
}

std::string frame(Framing framing, std::string_view payload) {
  check_framable(framing, payload);
  if (framing == Framing::raw) return std::string(payload);
  if (framing == Framing::lines) return std::string(payload) + "\n";
  return prefixed_frame(payload, head_size(framing));
}

Deframer::Deframer(Framing framing, std::size_t max_message) : framing_(framing), max_(max_message) {}

std::string Deframer::push(std::string_view bytes, std::deque<Message>& out) {
  if (framing_ == Framing::raw) {
    if (!bytes.empty()) out.push_back({std::string(bytes), false});
    return {};
  }
  buffer_.append(bytes);
  return framing_ == Framing::lines ? lines(out) : prefixed(out);
}

std::string Deframer::lines(std::deque<Message>& out) {
  std::size_t start = 0;
  for (auto nl = buffer_.find('\n'); nl != std::string::npos; nl = buffer_.find('\n', start)) {
    std::size_t end = nl;
    if (end > start && buffer_[end - 1] == '\r') --end;
    if (end - start > max_) return too_big(end - start);
    out.push_back({buffer_.substr(start, end - start), true});
    start = nl + 1;
  }
  buffer_.erase(0, start);
  if (buffer_.size() > max_ + 1) return "peer sent a line over maxMessageBytes " + size_text(max_) + " with no \\n";
  return {};
}

std::string Deframer::prefixed(std::deque<Message>& out) {
  const std::size_t head = head_size(framing_);
  std::size_t at = 0;
  while (buffer_.size() - at >= head) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < head; ++i) n = (n << 8) | static_cast<std::uint8_t>(buffer_[at + i]);
    if (n > max_) return too_big(n);
    if (buffer_.size() - at - head < n) break;
    out.push_back({buffer_.substr(at + head, n), false});
    at += head + n;
  }
  buffer_.erase(0, at);
  return {};
}

std::string Deframer::too_big(std::size_t size) const {
  return "peer sent " + size_text(size) + " message, over maxMessageBytes " + size_text(max_);
}

}  // namespace alpha::net
