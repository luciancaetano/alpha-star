#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/input.hpp"
#include "core/tags.hpp"

using namespace alpha::core;

namespace {
InputSet pressing(std::initializer_list<const char*> names) {
  InputSet set;
  for (const char* n : names) set.set(*parse_input(n));
  return set;
}
}  // namespace

TEST_CASE("parse_input knows keys, mouse and gamepad names") {
  CHECK(parse_input("Space"));
  CHECK(parse_input("R"));
  CHECK(parse_input("MouseLeft"));
  CHECK(parse_input("GamepadA"));
  CHECK_FALSE(parse_input("Spacebar"));
  CHECK(parse_input("A") != parse_input("B"));
}

TEST_CASE("Actions report pressed, down and released per step") {
  Actions actions;
  const std::vector<std::string> inputs{"Space", "MouseLeft"};
  const ActionId flap = actions.bind("flap", inputs);
  CHECK(actions.find("flap") == flap);

  actions.update(pressing({"MouseLeft"}));
  CHECK(actions.pressed(flap));
  CHECK(actions.down(flap));
  actions.update(pressing({"MouseLeft", "Space"}));
  CHECK_FALSE(actions.pressed(flap));
  CHECK(actions.down(flap));
  actions.update({});
  CHECK(actions.released(flap));
  CHECK_FALSE(actions.down(flap));
}

TEST_CASE("Actions reject unknown inputs, naming the valid ones") {
  Actions actions;
  const std::vector<std::string> inputs{"Spacebar"};
  try {
    actions.bind("flap", inputs);
    FAIL("expected Error");
  } catch (const Error& e) {
    const std::string msg = e.what();
    CHECK(msg.find("\"Spacebar\"") != std::string::npos);
    CHECK(msg.find("Space") != std::string::npos);
    CHECK(msg.find("GamepadA") != std::string::npos);
  }
  CHECK_FALSE(actions.find("flap"));
}

TEST_CASE("TagRegistry interns names once and caps the count") {
  TagRegistry tags;
  const TagId bird = tags.intern("bird");
  CHECK(tags.intern("bird") == bird);
  CHECK(tags.find("bird") == bird);
  CHECK(tags.name(bird) == "bird");
  CHECK_FALSE(tags.find("pipe"));
  for (std::size_t i = 1; i < kMaxTags; ++i) tags.intern("t" + std::to_string(i));
  CHECK_THROWS_AS(tags.intern("one-too-many"), Error);
}
