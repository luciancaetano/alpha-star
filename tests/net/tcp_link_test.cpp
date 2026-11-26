#include <doctest/doctest.h>

#include "net/tcp_link.hpp"

using namespace alpha::net;

namespace {

// A byte stream driven by the test: what the "server" sends and what was written.
struct Wire {
  StreamState state = StreamState::connecting;
  std::string incoming, written, error;
};

class MemoryStream final : public ByteStream {
 public:
  explicit MemoryStream(Wire& w) : w_(w) {}
  StreamState pump(std::string& in) override {
    in += w_.incoming;
    w_.incoming.clear();
    return w_.state;
  }
  void write(std::string_view bytes) override { w_.written += bytes; }
  const std::string& error() const override { return w_.error; }

 private:
  Wire& w_;
};

}  // namespace

TEST_CASE("tcp link frames writes, deframes reads and follows the stream state") {
  Wire w;
  TcpLink link(std::make_unique<MemoryStream>(w), Framing::lines, 100);
  std::deque<Message> in;
  CHECK(link.pump(in).state == State::connecting);
  w.state = StreamState::open;
  w.incoming = "hi\nthere\n";
  CHECK(link.pump(in).state == State::open);
  REQUIRE(in.size() == 2);
  CHECK(in[1].data == "there");
  std::deque<Message> out{{"a", true}, {"b", true}};
  link.write(out);
  CHECK(out.empty());
  CHECK(w.written == "a\nb\n");
}

TEST_CASE("tcp link closes with the stream's error, or the framing error") {
  Wire w;
  w.state = StreamState::closed;
  w.error = "connect to h:1 refused";
  TcpLink refused(std::make_unique<MemoryStream>(w), Framing::u32, 100);
  std::deque<Message> in;
  const LinkStatus s = refused.pump(in);
  CHECK(s.state == State::closed);
  CHECK(s.error == "connect to h:1 refused");
  CHECK(refused.pump(in).state == State::closed);

  Wire big;
  big.state = StreamState::open;
  big.incoming = std::string("\x00\x00\x10\x00", 4);
  TcpLink link(std::make_unique<MemoryStream>(big), Framing::u32, 100);
  CHECK(link.pump(in).error == "peer sent 4 KiB message, over maxMessageBytes 100 bytes");
}

TEST_CASE("tcp link: a local close is clean") {
  Wire w;
  w.state = StreamState::open;
  TcpLink link(std::make_unique<MemoryStream>(w), Framing::raw, 100);
  link.close(1000, "");
  std::deque<Message> in;
  const LinkStatus s = link.pump(in);
  CHECK(s.state == State::closed);
  CHECK(s.error.empty());
}
