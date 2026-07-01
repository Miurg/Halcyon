#pragma once

#include "HalcyonExport.hpp"
#include <glm/fwd.hpp>

struct HALCYON_API PrimitivesInfo
{
	uint32_t vertexOffset = -1;
	uint32_t indexOffset = -1;
	uint32_t indexCount = -1;
	uint32_t materialIndex = -1;
	glm::vec3 AABBMin;
	glm::vec3 AABBMax;
};