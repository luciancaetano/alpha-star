#include "net/fake_net.hpp"

#include <functional>

#include "core/error.hpp"

namespace alpha::net {

// One connection: messages in flight each way, with steps left before arrival.
struct FakeNet::Pipe {
  Kind kind = Kind::tcp;
  int latency = 0;
  State state = State::connecting;
  std::string error;
  std::optional<int> close_code;
  int opens_in = 0;
  bool told_connect = false, told_close = false;
  std::deque<std::pair<int, Message>> to_game, to_server;
};

namespace {

// Messages whose countdown reached zero, in order (one tick per call).
void arrive(std::deque<std::pair<int, Message>>& q, const std::function<void(Message&&)>& take) {
  for (auto& [left, m] : q) --left;
  while (!q.empty() && q.front().first <= 0) {
    take(std::move(q.front().second));
    q.pop_front();
  }
}

class FakeLink final : public Link {
 public:
  explicit FakeLink(std::shared_ptr<FakeNet::Pipe> pipe) : p_(std::move(pipe)) {}

  LinkStatus pump(std::deque<Message>& in) override {
    if (p_->state == State::connecting && --p_->opens_in <= 0) p_->state = State::open;
    arrive(p_->to_game, [&](Message&& m) { in.push_back(std::move(m)); });
    if (p_->state == State::closing && p_->to_game.empty()) p_->state = State::closed;
    return {p_->state, p_->error, p_->close_code};
  }
  void write(std::deque<Message>& out) override {
    for (Message& m : out) p_->to_server.emplace_back(p_->latency, std::move(m));
    out.clear();
  }
  void close(int code, const std::string&) override {
    if (p_->state == State::closed) return;
    p_->state = State::closed;
    p_->close_code = p_->kind == Kind::ws ? std::optional<int>(code) : std::nullopt;
  }

 private:
  std::shared_ptr<FakeNet::Pipe> p_;
};

// A socket whose endpoint has no server: closed with the refusal on its first pump.
class RefusedLink final : public Link {
 public:
  explicit RefusedLink(std::string error) : error_(std::move(error)) {}
  LinkStatus pump(std::deque<Message>&) override { return {State::closed, error_, std::nullopt}; }
  void write(std::deque<Message>& out) override { out.clear(); }
  void close(int, const std::string&) override {}

 private:
  std::string error_;
};

}  // namespace

FakeNet::FakeNet() = default;
FakeNet::~FakeNet() = default;

std::uint32_t FakeNet::server(Kind kind, const std::string& endpoint, Options options) {
  if (by_endpoint_.contains(endpoint)) throw core::Error("netFake.server: " + endpoint + " already has a server");
  const std::uint32_t id = next_id_++;
  by_endpoint_.emplace(endpoint, id);
  servers_.emplace(id, Server{kind, options, {}});
  return id;
}

std::unique_ptr<Link> FakeNet::connect(const Address& address) {
  const std::string endpoint = endpoint_of(address);
  const auto it = by_endpoint_.find(endpoint);
  if (it == by_endpoint_.end() || servers_.at(it->second).kind != address.kind) {
    return std::make_unique<RefusedLink>("connect to " + endpoint + " refused (no netFake server there)");
  }
  Server& srv = servers_.at(it->second);
  auto pipe = std::make_shared<Pipe>();
  pipe->kind = address.kind;
  pipe->latency = srv.options.latency_steps;
  pipe->opens_in = srv.options.latency_steps;
  if (address.kind == Kind::udp) pipe->state = State::open;
  const std::uint32_t id = next_id_++;
  pipes_.emplace(id, pipe);
  srv.peers.push_back(id);
  return std::make_unique<FakeLink>(pipe);
}

LinkFactory FakeNet::factory() {
  return [this](const Address& address) { return connect(address); };
}

FakeNet::Events FakeNet::poll(std::uint32_t server) {
  const auto it = servers_.find(server);
  if (it == servers_.end()) throw core::Error("netFake: unknown server #" + std::to_string(server));
  Events out;
  const double loss = it->second.kind == Kind::udp ? it->second.options.loss : 0;
  for (const std::uint32_t peer : it->second.peers) poll_peer(peer, loss, out);
  return out;
}

void FakeNet::poll_peer(std::uint32_t peer, double loss, Events& out) {
  Pipe& p = *pipes_.at(peer);
  if (!p.told_connect && p.state != State::connecting) p.told_connect = true, out.connects.push_back(peer);
  arrive(p.to_server, [&](Message&& m) {
    if (loss <= 0 || random_.range(0, 1) >= loss) out.messages.emplace_back(peer, std::move(m));
  });
  if (!p.told_close && p.state == State::closed) p.told_close = true, out.closes.push_back(peer);
}

FakeNet::Pipe& FakeNet::pipe(std::uint32_t peer) {
  if (!pipes_.contains(peer)) throw core::Error("netFake: unknown peer #" + std::to_string(peer));
  return *pipes_.at(peer);
}

bool FakeNet::send(std::uint32_t peer, Message message) {
  Pipe& p = pipe(peer);
  if (p.state == State::closed || p.state == State::closing) return false;
  p.to_game.emplace_back(p.latency, std::move(message));
  return true;
}

void FakeNet::close(std::uint32_t peer, int code) {
  Pipe& p = pipe(peer);
  if (p.state == State::closed || p.state == State::closing) return;
  p.state = State::closing;  // messages already sent still arrive first
  if (p.kind == Kind::ws) p.close_code = code;
}

}  // namespace alpha::net
