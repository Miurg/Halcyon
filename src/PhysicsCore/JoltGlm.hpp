#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline glm::vec3 toGlm(const JPH::Vec3& v)
{
	return {v.GetX(), v.GetY(), v.GetZ()};
}
inline glm::quat toGlm(const JPH::Quat& q)
{
	return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

inline JPH::Vec3 toJolt(const glm::vec3& v)
{
	return JPH::Vec3(v.x, v.y, v.z);
}
inline JPH::Quat toJolt(const glm::quat& q)
{
	return JPH::Quat(q.x, q.y, q.z, q.w);
}

#ifdef JPH_DOUBLE_PRECISION
inline glm::vec3 toGlm(const JPH::RVec3& v)
{
	return {float(v.GetX()), float(v.GetY()), float(v.GetZ())};
}
inline JPH::RVec3 toJoltR(const glm::vec3& v)
{
	return JPH::RVec3(v.x, v.y, v.z);
}
#endif