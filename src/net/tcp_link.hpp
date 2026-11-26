#pragma once

#include <memory>

#include "net/byte_stream.hpp"
#include "net/link.hpp"

namespace alpha::net {

// net.tcp: messages framed (net/framing) over a byte stream.
class TcpLink final : public Link {
 public:
  TcpLink(std::unique_ptr<ByteStream> stream, Framing framing, std::size_t max_message);
  LinkStatus pump(std::deque<Message>& in) override;
  void write(std::deque<Message>& out) override;
  void close(int code, const std::string& reason) override;

 private:
  std::unique_ptr<ByteStream> stream_;
  Framing framing_;
  Deframer deframer_;
  std::string error_;
  bool closed_ = false;
};

}  // namespace alpha::net
