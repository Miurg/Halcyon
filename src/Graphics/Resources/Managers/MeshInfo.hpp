#pragma once

#include "../../VulkanConst.hpp"

struct MeshInfo
{
	uint32_t vertexOffset = -1;
	uint32_t indexOffset = -1;
	uint32_t indexCount = -1;
	uint32_t bufferIndex = -1;
	uint32_t entitiesSubscribed = -1;
	char path[MAX_PATH_LEN];
};