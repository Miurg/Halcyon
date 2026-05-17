#pragma once

#include "PhysLayers.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <span>
#include <cstdint>
#include <variant>

struct Sphere
{
	float radius;
};

struct Box
{
	glm::vec3 halfExtents;
};

struct Capsule
{
	float halfHeight;
	float radius;
};

struct Cylinder
{
	float halfHeight;
	float radius;
};

struct ConvexHull
{
	std::span<const glm::vec3> points;
};

using Shape = std::variant<Sphere, Box, Capsule, Cylinder, ConvexHull>;

enum class Motion
{
	Static,
	Kinematic,
	Dynamic
};

struct Body
{
	glm::vec3 pos         = {0, 0, 0};
	glm::quat rot         = {1, 0, 0, 0};
	Motion    motion      = Motion::Dynamic;
	uint16_t  layer       = Layers::MOVING;
	float     friction    = 0.2f;
	float     restitution = 0.0f;
};