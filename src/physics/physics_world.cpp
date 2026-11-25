#include "physics/physics_world.hpp"

#include <algorithm>
#include <variant>

#include "physics/queries.hpp"
#include "physics/units.hpp"

namespace alpha::physics {

namespace {

constexpr int kSubSteps = 4;

// Shape user data marking a one-way collider (compared by address).
constexpr char kOneWay = 0;

b2BodyType body_type(core::BodyType type) {
  switch (type) {
    case core::BodyType::Dynamic: return b2_dynamicBody;
    case core::BodyType::Kinematic: return b2_kinematicBody;
    case core::BodyType::Static: break;
  }
  return b2_staticBody;
}

b2ShapeDef shape_def(const core::ColliderSpec& collider, core::CollisionFilter filter) {
  b2ShapeDef def = b2DefaultShapeDef();
  def.isSensor = collider.sensor;
  def.enableSensorEvents = true;
  def.enableContactEvents = true;
  def.enableHitEvents = true;
  def.material.friction = collider.material.friction;
  def.material.restitution = collider.material.restitution;
  def.density = collider.material.density;
  def.filter.categoryBits = filter.category;
  def.filter.maskBits = filter.mask;
  def.enablePreSolveEvents = collider.one_way;
  def.userData = collider.one_way ? const_cast<char*>(&kOneWay) : nullptr;
  return def;
}

void add_collider(b2BodyId body, const core::BodySpec& spec) {
  const b2ShapeDef def = shape_def(spec.collider, spec.filter);
  if (const auto* circle = std::get_if<core::Circle>(&spec.collider.geometry)) {
    const b2Circle shape{{0.0f, 0.0f}, circle->radius / kPixelsPerMeter};
    b2CreateCircleShape(body, &def, &shape);
    return;
  }
  const core::Vec2 half = std::get<core::Rect>(spec.collider.geometry).size * 0.5f;
  const b2Polygon box = b2MakeBox(half.x / kPixelsPerMeter, half.y / kPixelsPerMeter);
  b2CreatePolygonShape(body, &def, &box);
}

bool one_way(b2ShapeId shape) { return b2Shape_GetUserData(shape) == &kOneWay; }

// A one-way collider only holds what comes from above: the normal from it to
// the other body points up (-y). Pure: Box2D may call it from its solver.
bool pre_solve(b2ShapeId a, b2ShapeId b, b2Manifold* manifold, void*) {
  if (one_way(a)) return manifold->normal.y < -0.5f;
  if (one_way(b)) return manifold->normal.y > 0.5f;
  return true;
}

Hit hit_of(const b2ContactBeginTouchEvent& e) {
  const b2Manifold& m = e.manifold;
  Hit hit{owner_of(e.shapeIdA), owner_of(e.shapeIdB), {m.normal.x, m.normal.y}, {}, 0.0f};
  for (int i = 0; i < m.pointCount; ++i) {
    hit.point = hit.point + to_pixels(m.points[i].point) * (1.0f / static_cast<float>(m.pointCount));
  }
  return hit;
}

bool same_pair(const b2ContactHitEvent& h, b2ShapeId a, b2ShapeId b) {
  return (B2_ID_EQUALS(h.shapeIdA, a) && B2_ID_EQUALS(h.shapeIdB, b)) ||
         (B2_ID_EQUALS(h.shapeIdA, b) && B2_ID_EQUALS(h.shapeIdB, a));
}

// Approach speed of the pair from the step's hit events (0 when it only slid into contact).
float approach_speed(const b2ContactEvents& contacts, b2ShapeId a, b2ShapeId b) {
  for (int i = 0; i < contacts.hitCount; ++i) {
    if (same_pair(contacts.hitEvents[i], a, b)) return contacts.hitEvents[i].approachSpeed * kPixelsPerMeter;
  }
  return 0.0f;
}

bool both_valid(b2ShapeId a, b2ShapeId b) { return b2Shape_IsValid(a) && b2Shape_IsValid(b); }

void contact_events(b2WorldId world, StepContacts& out) {
  const b2ContactEvents contacts = b2World_GetContactEvents(world);
  for (int i = 0; i < contacts.beginCount; ++i) {
    const b2ContactBeginTouchEvent& e = contacts.beginEvents[i];
    out.hits.push_back(hit_of(e));
    out.hits.back().speed = approach_speed(contacts, e.shapeIdA, e.shapeIdB);
  }
  for (int i = 0; i < contacts.endCount; ++i) {
    const b2ContactEndTouchEvent& e = contacts.endEvents[i];
    if (both_valid(e.shapeIdA, e.shapeIdB)) out.hit_ends.push_back({owner_of(e.shapeIdA), owner_of(e.shapeIdB)});
  }
}

void sensor_events(b2WorldId world, StepContacts& out) {
  const b2SensorEvents sensors = b2World_GetSensorEvents(world);
  for (int i = 0; i < sensors.beginCount; ++i) {
    const b2SensorBeginTouchEvent& e = sensors.beginEvents[i];
    out.sensor_enters.push_back({owner_of(e.visitorShapeId), owner_of(e.sensorShapeId)});
  }
  for (int i = 0; i < sensors.endCount; ++i) {
    const b2SensorEndTouchEvent& e = sensors.endEvents[i];
    if (both_valid(e.visitorShapeId, e.sensorShapeId)) {
      out.sensor_exits.push_back({owner_of(e.visitorShapeId), owner_of(e.sensorShapeId)});
    }
  }
}

}  // namespace

PhysicsWorld::PhysicsWorld(core::Vec2 gravity) {
  b2WorldDef def = b2DefaultWorldDef();
  def.gravity = to_meters(gravity);
  def.hitEventThreshold = 0.0f;  // every approach reports its speed
  world_ = b2CreateWorld(&def);
  b2World_SetPreSolveCallback(world_, pre_solve, nullptr);
}

PhysicsWorld::~PhysicsWorld() { b2DestroyWorld(world_); }

b2BodyId PhysicsWorld::create(std::uint32_t owner, const core::BodySpec& spec, core::Vec2 pos,
                              float rotation) {
  b2BodyDef def = b2DefaultBodyDef();
  def.type = body_type(spec.type);
  def.position = to_meters(pos);
  def.rotation = b2MakeRot(rotation * kDegToRad);
  def.linearVelocity = to_meters(spec.velocity);
  def.fixedRotation = spec.fixed_rotation;
  def.gravityScale = spec.gravity_scale;
  def.isBullet = spec.bullet;
  def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(owner));
  const b2BodyId body = b2CreateBody(world_, &def);
  add_collider(body, spec);
  return body;
}

void PhysicsWorld::destroy(b2BodyId body) { b2DestroyBody(body); }

StepContacts PhysicsWorld::step(float dt) {
  b2World_Step(world_, dt, kSubSteps);
  StepContacts out;
  contact_events(world_, out);
  sensor_events(world_, out);
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

float PhysicsWorld::angular_velocity(b2BodyId body) { return b2Body_GetAngularVelocity(body) / kDegToRad; }

void PhysicsWorld::set_angular_velocity(b2BodyId body, float degrees_per_second) {
  b2Body_SetAngularVelocity(body, degrees_per_second * kDegToRad);
}

float PhysicsWorld::mass(b2BodyId body) { return b2Body_GetMass(body); }

void PhysicsWorld::apply_force(b2BodyId body, core::Vec2 force) {
  b2Body_ApplyForceToCenter(body, to_meters(force), true);
}

void PhysicsWorld::apply_impulse(b2BodyId body, core::Vec2 impulse) {
  b2Body_ApplyLinearImpulseToCenter(body, to_meters(impulse), true);
}

void PhysicsWorld::apply_torque(b2BodyId body, float torque) {
  b2Body_ApplyTorque(body, torque / (kPixelsPerMeter * kPixelsPerMeter), true);
}

}  // namespace alpha::physics
