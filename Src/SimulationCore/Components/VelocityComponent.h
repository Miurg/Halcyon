#pragma once

#include "../../Core//Components/ComponentManager.h"
#include <glm/ext/vector_float3.hpp>

struct VelocityComponent : Component
{
	glm::vec3 Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	VelocityComponent(glm::vec3 vel) : Velocity(vel) {}
};
