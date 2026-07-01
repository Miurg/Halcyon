#pragma once

#include "HalcyonExport.hpp"
#include "PhysicsCore/PhysLayers.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <span>
#include <cstdint>
#include <variant>

struct HALCYON_API Sphere
{
	float radius;
};

struct HALCYON_API Box
{
	glm::vec3 halfExtents;
};

struct HALCYON_API Capsule
{
	float halfHeight;
	float radius;
};

struct HALCYON_API Cylinder
{
	float halfHeight;
	float radius;
};

struct HALCYON_API ConvexHull
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

struct HALCYON_API Body
{
	glm::vec3 pos         = {0, 0, 0};
	glm::quat rot         = {1, 0, 0, 0};
	Motion    motion      = Motion::Dynamic;
	uint16_t  layer       = Layers::MOVING;
	float     friction    = 0.2f;
	float     restitution = 0.0f;
};