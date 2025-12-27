#pragma once

#include "../../VulkanConst.hpp"

struct MeshInfoComponent
{
	uint32_t vertexOffset;
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t bufferIndex;
	char path[MAX_PATH_LEN];
};