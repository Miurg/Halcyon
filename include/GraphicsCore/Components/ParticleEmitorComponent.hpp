#pragma once

#include "HalcyonExport.hpp"
#include <glm/glm.hpp>

struct HALCYON_API ParticleEmitorComponent
{
	bool active = true;
	uint32_t spawnCount = 10;
	float emissionAccumulator;
	glm::vec3 directionalVector = {0.0f, 1.0f, 0.0f};
	glm::vec2 spawnRadius = {1.0f, 1.0f}; // 1-min, 2-max
	glm::vec2 timeToLive = {1.0f, 1.0f};  // 1-min, 2-max
	glm::vec2 velocity = {0.0f, 0.0f};    // 1-min, 2-max
	glm::vec2 scale = {1.0f, 1.0f};       // 1-min, 2-max
};