#include <doctest/doctest.h>

#include <chrono>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "ecs/world.hpp"

using namespace alpha::core;
using alpha::ecs::EntityId;
using alpha::ecs::World;

namespace {

constexpr TagId kBird = 0;
constexpr TagId kSolid = 1;
constexpr TagId kGoal = 2;

TagSet tagged(std::initializer_list<TagId> ids) {
  TagSet s;
  for (TagId t : ids) s.set(t);
  return s;
}

EntitySpec plain(std::initializer_list<TagId> tags, Vec2 pos, int z = 0) {
  return {tagged(tags), pos, 0.0f, z, ShapeSpec{Circle{5}, Color{}, {}, {}}, {}, {}, {}};
}

EntitySpec solid_box(Vec2 pos, Vec2 size) {
  return {tagged({kSolid}), pos, 0.0f, 0, {}, BodySpec{BodyType::Static, {Rect{size}, false}, {}}, {}, {}};
}

EntitySpec falling_bird(Vec2 pos) {
  return {tagged({kBird}), pos, 0.0f, 0, {}, BodySpec{BodyType::Dynamic, {Circle{10}, false}, {}}, {}, {}};
}

}  // namespace

TEST_CASE("a world holds at most kMaxEntities; destroyed ones count until the step frees them") {
  World w({0, 0});
  const EntitySpec spec = plain({kBird}, {0, 0});
  EntityId first = 0;
  for (std::size_t i = 0; i < alpha::core::kMaxEntities; ++i) {
    const EntityId id = w.spawn(spec);
    if (i == 0) first = id;
  }
  CHECK_THROWS_WITH(w.spawn(spec), doctest::Contains("at most 100000 entities"));
  w.destroy(first);
  CHECK_THROWS(w.spawn(spec));
  const auto events = w.step(1.0f / 60.0f);
  CHECK(events.freed == std::vector<EntityId>{first});
  CHECK_NOTHROW(w.spawn(spec));
}

TEST_CASE("moving every entity once is linear in the world size (children looked up among linked ones)") {
  World w({0, 0});
  std::vector<EntityId> ids;
  for (int i = 0; i < 10000; ++i) ids.push_back(w.spawn(plain({kBird}, {0, 0})));
  const auto start = std::chrono::steady_clock::now();
  for (const EntityId id : ids) w.set_position(id, {1, 2});
  CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));  // was O(n^2): ~1e8 visits + sorts
  CHECK(w.position(ids.back()).y == 2.0f);
}

TEST_CASE("find returns the oldest living entity; find_all keeps creation order") {
  World w({0, 0});
  const EntityId a = w.spawn(plain({kSolid}, {1, 1}));
  const EntityId b = w.spawn(plain({kSolid, kGoal}, {2, 2}));
  w.spawn(plain({kBird}, {3, 3}));
  CHECK(w.find(kSolid) == a);
  CHECK(w.find_all(kSolid) == std::vector<EntityId>{a, b});
  CHECK(w.find_all(kGoal) == std::vector<EntityId>{b});
  CHECK(w.tags(b) == tagged({kSolid, kGoal}));
  w.destroy(a);
  CHECK(w.find(kSolid) == b);
}

TEST_CASE("destroy kills now; dead and stale ids throw a readable error") {
  World w({0, 0});
  const EntityId a = w.spawn(plain({kBird}, {1, 1}));
  w.destroy(a);
  w.destroy(a);  // idempotent
  CHECK_FALSE(w.alive(a));
  CHECK_FALSE(w.find(kBird));
  const std::string expected = "entity " + std::to_string(a) + " is not alive";
  CHECK_THROWS_WITH_AS(w.position(a), expected.c_str(), Error);
  w.step(1.0f / 60);
  const EntityId reused = w.spawn(plain({kBird}, {1, 1}));
  CHECK(reused != a);
  CHECK_FALSE(w.alive(a));
}

TEST_CASE("position and rotation work with and without a body") {
  World w({0, 0});
  const EntityId plain_id = w.spawn(plain({kGoal}, {1, 1}));
  const EntityId body_id = w.spawn(falling_bird({0, 0}));
  w.set_position(plain_id, {5, 6});
  w.set_rotation(plain_id, 45);
  CHECK(w.position(plain_id) == Vec2{5, 6});
  CHECK(w.rotation(plain_id) == 45);
  w.set_position(body_id, {40, 50});
  w.set_rotation(body_id, 10);
  w.step(1.0f / 60);
  CHECK(w.position(body_id).x == doctest::Approx(40));
  CHECK(w.rotation(body_id) == doctest::Approx(10).epsilon(0.01));  // b2MakeRot is approximate
}

TEST_CASE("velocity needs a body and says so") {
  World w({0, 0});
  const EntityId plain_id = w.spawn(plain({kGoal}, {1, 1}));
  CHECK_FALSE(w.has_body(plain_id));
  CHECK_THROWS_WITH_AS(w.velocity(plain_id),
                       doctest::Contains("has no body; spawn it with `body`"), Error);
  const EntityId bird = w.spawn(falling_bird({0, 0}));
  w.set_velocity(bird, {0, -320});
  CHECK(w.velocity(bird).y == doctest::Approx(-320));
}

TEST_CASE("step syncs physics and reports hits between living entities") {
  World w({0, 900});
  const EntityId ground = w.spawn(solid_box({100, 200}, {200, 20}));
  const EntityId bird = w.spawn(falling_bird({100, 100}));
  std::vector<alpha::physics::Hit> hits;
  for (int i = 0; i < 120 && hits.empty(); ++i) hits = w.step(1.0f / 60).hits;
  REQUIRE(hits.size() == 1);
  CHECK(((hits[0].a == bird && hits[0].b == ground) || (hits[0].a == ground && hits[0].b == bird)));
  CHECK(w.position(bird).y > 150);
}

TEST_CASE("draw_items orders by z then creation and skips destroyed") {
  World w({0, 0});
  w.spawn(plain({kSolid}, {1, 0}, 10));
  w.spawn(plain({kSolid}, {2, 0}, 0));
  const EntityId gone = w.spawn(plain({kSolid}, {3, 0}, 0));
  w.spawn(plain({kSolid}, {4, 0}, 0));
  w.destroy(gone);
  const EntityId last = w.spawn(plain({kSolid}, {5, 0}, 0));
  w.set_z(last, 20);
  CHECK(w.z(last) == 20);
  const auto items = w.draw_items();
  REQUIRE(items.size() == 4);
  CHECK(items[0].pos.x == 2);
  CHECK(items[1].pos.x == 4);
  CHECK(items[2].pos.x == 1);
  CHECK(items[3].pos.x == 5);
}

TEST_CASE("entities lists every living entity oldest first") {
  World w({0, 0});
  const EntityId a = w.spawn(plain({kSolid}, {1, 1}));
  const EntityId b = w.spawn(plain({}, {2, 2}));
  const EntityId c = w.spawn(plain({kBird}, {3, 3}));
  w.destroy(b);
  CHECK(w.entities() == std::vector<EntityId>{a, c});
}

TEST_CASE("colliders reports each living body's collider where the body is") {
  World w({0, 0});
  w.spawn(plain({}, {0, 0}));
  const EntityId box = w.spawn(solid_box({40, 50}, {20, 10}));
  const auto all = w.colliders();
  REQUIRE(all.size() == 1);
  CHECK(all[0].pos == Vec2{40, 50});
  CHECK(std::get<Rect>(all[0].collider.geometry).size == Vec2{20, 10});
  CHECK_FALSE(all[0].collider.sensor);
  w.destroy(box);
  CHECK(w.colliders().empty());
}

TEST_CASE("draw_items blends previous and current transforms by alpha") {
  World w({0, 0});
  const EntityId moved = w.spawn(plain({kSolid}, {0, 0}));
  w.snapshot();
  w.set_position(moved, {10, 20});
  w.set_rotation(moved, 90);
  const EntityId fresh = w.spawn(plain({kGoal}, {50, 50}));  // spawned after the snapshot
  const auto half = w.draw_items(0.5f);
  REQUIRE(half.size() == 2);
  CHECK(half[0].pos == Vec2{5, 10});
  CHECK(half[0].rotation == doctest::Approx(45));
  CHECK(half[1].pos == Vec2{50, 50});  // no smear from the origin
  CHECK(w.draw_items(1.0f)[0].pos == Vec2{10, 20});
  CHECK(w.draw_items(0.0f)[0].pos == Vec2{0, 0});
  static_cast<void>(fresh);
}

TEST_CASE("rotation blends the short way around") {
  World w({0, 0});
  const EntityId e = w.spawn({TagSet{}, {0, 0}, 350.0f, 0, ShapeSpec{Circle{5}, Color{}, {}, {}}, {}, {}, {}});
  w.snapshot();
  w.set_rotation(e, 10);
  CHECK(w.draw_items(0.5f)[0].rotation == doctest::Approx(360).epsilon(0.001));
}

TEST_CASE("physics motion interpolates between steps") {
  World w({0, 0});
  EntitySpec spec = plain({kBird}, {0, 0});
  spec.body = BodySpec{BodyType::Kinematic, {Circle{5}, false}, {60, 0}};
  w.spawn(spec);
  w.snapshot();
  w.step(1.0f / 60);  // moves 1 px
  CHECK(w.draw_items(0.5f)[0].pos.x == doctest::Approx(0.5f));
}
