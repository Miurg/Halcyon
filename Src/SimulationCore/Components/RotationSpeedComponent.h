#pragma once
#include "../../Core/Components/ComponentManager.h"
#include <glm/ext/vector_float3.hpp>
struct RotationSpeedComponent : Component
{
	float Speed = 0.0f;
	glm::vec3 Axis = glm::vec3(0.0f, 1.0f, 0.0f);
	RotationSpeedComponent(float s, glm::vec3 ax) : Speed(s), Axis(ax) {}
};