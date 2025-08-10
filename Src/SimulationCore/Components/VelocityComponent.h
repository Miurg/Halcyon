#pragma once

#include "../../Core//Components/ComponentManager.h"
#include <glm/ext/vector_float3.hpp>

struct VelocityComponent : Component
{
	glm::vec3 Velocity;
	VelocityComponent(glm::vec3 vel) : Velocity(vel) {}
};
