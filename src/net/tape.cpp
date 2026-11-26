#include "net/tape.hpp"

#include <sstream>

#include "core/error.hpp"

namespace alpha::net {

namespace {

std::string hex(std::string_view bytes) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out = "x";  // never empty, so every field is one token
  for (const char ch : bytes) {
    const auto c = static_cast<unsigned char>(ch);
    out += {kDigits[c >> 4], kDigits[c & 15]};
  }
  return out;
}

int nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

std::string unhex(const std::string& field, std::size_t line) {
  std::string out;
  bool ok = !field.empty() && field[0] == 'x' && field.size() % 2 == 1;
  for (std::size_t i = 1; ok && i < field.size(); i += 2) {
    ok = nibble(field[i]) >= 0 && nibble(field[i + 1]) >= 0;
    out.push_back(static_cast<char>(nibble(field[i]) * 16 + nibble(field[i + 1])));
  }
  if (!ok) throw core::Error("net tape line " + std::to_string(line) + ": bad hex field");
  return out;
}

State state_of(int n, std::size_t line) {
  if (n < 0 || n > 3) throw core::Error("net tape line " + std::to_string(line) + ": bad state");
  return static_cast<State>(n);
}

}  // namespace

void Tape::record(std::uint32_t socket, std::uint64_t step, const LinkStatus& status, std::vector<Message> messages) {
  const auto last = last_.find(socket);
  const bool changed = last == last_.end() || last->second.state != status.state || last->second.error != status.error ||
                       last->second.close_code != status.close_code;
  if (!changed && messages.empty()) return;
  last_[socket] = status;
  add({socket, step, status, std::move(messages)});
}

void Tape::add(Event e) {
  index_[{e.socket, e.step}] = events_.size();
  events_.push_back(std::move(e));
}

const Tape::Event* Tape::at(std::uint32_t socket, std::uint64_t step) const {
  const auto it = index_.find({socket, step});
  return it == index_.end() ? nullptr : &events_[it->second];
}

std::string Tape::text() const {
  std::string out = "alpha-net-tape 1\n";
  for (const Event& e : events_) {
    out += "E " + std::to_string(e.socket) + " " + std::to_string(e.step) + " " + std::to_string(static_cast<int>(e.status.state)) +
           " " + (e.status.close_code ? std::to_string(*e.status.close_code) : "-") + " " + hex(e.status.error) + "\n";
    for (const Message& m : e.messages) out += std::string("M ") + (m.text ? "t " : "b ") + hex(m.data) + "\n";
  }
  return out;
}

Tape Tape::parse(std::string_view text) {
  std::istringstream in{std::string(text)};
  std::string line;
  if (!std::getline(in, line) || line != "alpha-net-tape 1") throw core::Error("net tape: not an alpha-net-tape 1 file");
  Tape tape;
  for (std::size_t n = 2; std::getline(in, line); ++n) {
    std::istringstream f(line);
    std::string kind, a, b, c, d, e;
    f >> kind >> a >> b;
    if (kind == "M" && !tape.events_.empty()) {
      tape.events_.back().messages.push_back({unhex(b, n), a == "t"});
      continue;
    }
    f >> c >> d >> e;
    if (kind != "E" || e.empty()) throw core::Error("net tape line " + std::to_string(n) + ": expected E or M");
    const std::optional<int> code = d == "-" ? std::nullopt : std::optional<int>(std::stoi(d));
    tape.add({static_cast<std::uint32_t>(std::stoul(a)), std::stoull(b), {state_of(std::stoi(c), n), unhex(e, n), code}, {}});
  }
  return tape;
}

}  // namespace alpha::net
