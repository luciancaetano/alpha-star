#include "physics/joints.hpp"

#include "physics/units.hpp"

namespace alpha::physics {

namespace {

constexpr float kMeters = 1.0f / kPixelsPerMeter;

float angle_between(b2BodyId a, b2BodyId b) {
  return b2RelativeAngle(b2Body_GetRotation(b), b2Body_GetRotation(a));
}

// Revolute, weld and prismatic: both bodies held at spec.anchor_a, angles relative to now.
template <typename Def>
Def pinned(Def def, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  def.bodyIdA = a;
  def.bodyIdB = b;
  def.localAnchorA = b2Body_GetLocalPoint(a, to_meters(spec.anchor_a));
  def.localAnchorB = b2Body_GetLocalPoint(b, to_meters(spec.anchor_a));
  def.referenceAngle = angle_between(a, b);
  def.collideConnected = spec.collide;
  return def;
}

b2JointId revolute(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  b2RevoluteJointDef def = pinned(b2DefaultRevoluteJointDef(), a, b, spec);
  const Limits turn = spec.limits.value_or(Limits{});  // degrees
  def.enableLimit = spec.limits.has_value();
  def.lowerAngle = turn.lower * kDegToRad;
  def.upperAngle = turn.upper * kDegToRad;
  def.enableMotor = spec.motor.has_value();
  def.motorSpeed = spec.motor.value_or(Motor{}).speed * kDegToRad;
  def.maxMotorTorque = spec.motor.value_or(Motor{}).max * kMeters * kMeters;
  return b2CreateRevoluteJoint(world, &def);
}

b2JointId distance(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  b2DistanceJointDef def = b2DefaultDistanceJointDef();
  def.bodyIdA = a;
  def.bodyIdB = b;
  def.localAnchorA = b2Body_GetLocalPoint(a, to_meters(spec.anchor_a));
  def.localAnchorB = b2Body_GetLocalPoint(b, to_meters(spec.anchor_b));
  def.length = spec.length ? *spec.length * kMeters : b2Distance(to_meters(spec.anchor_a), to_meters(spec.anchor_b));
  def.collideConnected = spec.collide;
  if (spec.limits) {
    def.enableLimit = true;
    def.minLength = spec.limits->lower * kMeters;
    def.maxLength = spec.limits->upper * kMeters;
  }
  if (spec.type == JointType::Rope) {
    def.enableSpring = true;  // zero stiffness: free while shorter than the limit
    def.enableLimit = true;
    def.minLength = 0.0f;
    def.maxLength = def.length;
  }
  return b2CreateDistanceJoint(world, &def);
}

b2JointId weld(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  b2WeldJointDef def = pinned(b2DefaultWeldJointDef(), a, b, spec);
  return b2CreateWeldJoint(world, &def);
}

b2JointId prismatic(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  b2PrismaticJointDef def = pinned(b2DefaultPrismaticJointDef(), a, b, spec);
  def.localAxisA = b2Body_GetLocalVector(a, b2Normalize({spec.axis.x, spec.axis.y}));
  if (spec.limits) {  // pixels along the axis
    def.enableLimit = true;
    def.lowerTranslation = spec.limits->lower * kMeters;
    def.upperTranslation = spec.limits->upper * kMeters;
  }
  if (const auto& m = spec.motor) {
    def.enableMotor = true;
    def.motorSpeed = m->speed * kMeters;
    def.maxMotorForce = m->max * kMeters;
  }
  return b2CreatePrismaticJoint(world, &def);
}

}  // namespace

std::uint32_t Joints::create(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec) {
  b2JointId id{};
  switch (spec.type) {
    case JointType::Revolute: id = revolute(world, a, b, spec); break;
    case JointType::Distance:
    case JointType::Rope: id = distance(world, a, b, spec); break;
    case JointType::Weld: id = weld(world, a, b, spec); break;
    case JointType::Prismatic: id = prismatic(world, a, b, spec); break;
  }
  b2Body_SetAwake(a, true);
  b2Body_SetAwake(b, true);
  joints_.emplace(next_, id);
  return next_++;
}

void Joints::destroy(std::uint32_t handle) {
  const auto it = joints_.find(handle);
  if (it == joints_.end()) return;
  if (b2Joint_IsValid(it->second)) b2DestroyJoint(it->second);
  joints_.erase(it);
}

bool Joints::alive(std::uint32_t handle) const {
  const auto it = joints_.find(handle);
  return it != joints_.end() && b2Joint_IsValid(it->second);
}

int Joints::count() const {
  int n = 0;
  for (const auto& [handle, id] : joints_) n += b2Joint_IsValid(id) ? 1 : 0;
  return n;
}

}  // namespace alpha::physics
