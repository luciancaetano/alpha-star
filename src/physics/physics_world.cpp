#include "physics/physics_world.hpp"

#include <numbers>
#include <variant>

namespace alpha::physics {

namespace {

// Box2D is tuned for 0.1..10 m objects; game objects are ~5..500 px.
constexpr float kPixelsPerMeter = 50.0f;
constexpr int kSubSteps = 4;
constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

b2Vec2 to_meters(core::Vec2 v) { return {v.x / kPixelsPerMeter, v.y / kPixelsPerMeter}; }
core::Vec2 to_pixels(b2Vec2 v) { return {v.x * kPixelsPerMeter, v.y * kPixelsPerMeter}; }

b2BodyType body_type(core::BodyType type) {
  switch (type) {
    case core::BodyType::Dynamic: return b2_dynamicBody;
    case core::BodyType::Kinematic: return b2_kinematicBody;
    case core::BodyType::Static: break;
  }
  return b2_staticBody;
}

void add_collider(b2BodyId body, const core::ColliderSpec& collider) {
  b2ShapeDef def = b2DefaultShapeDef();
  def.isSensor = collider.sensor;
  def.enableSensorEvents = true;
  def.enableContactEvents = true;
  if (const auto* circle = std::get_if<core::Circle>(&collider.geometry)) {
    const b2Circle shape{{0.0f, 0.0f}, circle->radius / kPixelsPerMeter};
    b2CreateCircleShape(body, &def, &shape);
    return;
  }
  const core::Vec2 half = std::get<core::Rect>(collider.geometry).size * 0.5f;
  const b2Polygon box = b2MakeBox(half.x / kPixelsPerMeter, half.y / kPixelsPerMeter);
  b2CreatePolygonShape(body, &def, &box);
}

std::uint32_t owner_of(b2ShapeId shape) {
  const void* data = b2Body_GetUserData(b2Shape_GetBody(shape));
  return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(data));
}

}  // namespace

PhysicsWorld::PhysicsWorld(core::Vec2 gravity) {
  b2WorldDef def = b2DefaultWorldDef();
  def.gravity = to_meters(gravity);
  world_ = b2CreateWorld(&def);
}

PhysicsWorld::~PhysicsWorld() { b2DestroyWorld(world_); }

b2BodyId PhysicsWorld::create(std::uint32_t owner, const core::BodySpec& spec, core::Vec2 pos,
                              float rotation) {
  b2BodyDef def = b2DefaultBodyDef();
  def.type = body_type(spec.type);
  def.position = to_meters(pos);
  def.rotation = b2MakeRot(rotation * kDegToRad);
  def.linearVelocity = to_meters(spec.velocity);
  def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(owner));
  const b2BodyId body = b2CreateBody(world_, &def);
  add_collider(body, spec.collider);
  return body;
}

void PhysicsWorld::destroy(b2BodyId body) { b2DestroyBody(body); }

StepContacts PhysicsWorld::step(float dt) {
  b2World_Step(world_, dt, kSubSteps);
  StepContacts out;
  const b2ContactEvents contacts = b2World_GetContactEvents(world_);
  for (int i = 0; i < contacts.beginCount; ++i) {
    const b2ContactBeginTouchEvent& e = contacts.beginEvents[i];
    out.hits.push_back({owner_of(e.shapeIdA), owner_of(e.shapeIdB)});
  }
  const b2SensorEvents sensors = b2World_GetSensorEvents(world_);
  for (int i = 0; i < sensors.beginCount; ++i) {
    const b2SensorBeginTouchEvent& e = sensors.beginEvents[i];
    out.sensor_enters.push_back({owner_of(e.visitorShapeId), owner_of(e.sensorShapeId)});
  }
  return out;
}

core::Vec2 PhysicsWorld::position(b2BodyId body) { return to_pixels(b2Body_GetPosition(body)); }

float PhysicsWorld::rotation(b2BodyId body) {
  return b2Rot_GetAngle(b2Body_GetRotation(body)) / kDegToRad;
}

void PhysicsWorld::set_transform(b2BodyId body, core::Vec2 pos, float rotation) {
  b2Body_SetTransform(body, to_meters(pos), b2MakeRot(rotation * kDegToRad));
}

core::Vec2 PhysicsWorld::velocity(b2BodyId body) {
  return to_pixels(b2Body_GetLinearVelocity(body));
}

void PhysicsWorld::set_velocity(b2BodyId body, core::Vec2 velocity) {
  b2Body_SetLinearVelocity(body, to_meters(velocity));
}

}  // namespace alpha::physics
