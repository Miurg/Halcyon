#pragma once

#include <glm/glm.hpp>

struct ParticlePropertiesComponent
{
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	float _padding;
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};
	float timeToDie =- 1.0f;
};