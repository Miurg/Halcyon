#pragma once
#include <glm/glm.hpp>

struct ParticleEmitorComponent
{
	glm::vec3 directionalVector;
	glm::vec2 spawnRadius; // 1-min, 2-max
	glm::vec2 timeToLive;  // 1-min, 2-max
	glm::vec2 velocity;    // 1-min, 2-max
	glm::vec2 scale;       // 1-min, 2-max
};