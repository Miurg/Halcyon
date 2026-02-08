#pragma once

#include <glm/fwd.hpp>

struct PrimitivesInfo
{
	uint32_t vertexOffset = -1;
	uint32_t indexOffset = -1;
	uint32_t indexCount = -1;
	uint32_t textureIndex = -1; 
	glm::vec4 baseColorFactor;
	glm::vec3 AABBMin;
	glm::vec3 AABBMax;
};