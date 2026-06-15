#pragma once

#include "GraphicsCore/VulkanConst.hpp"
#include "GraphicsCore/Resources/Managers/PrimitivesInfo.hpp"

struct MeshInfo
{
	std::vector<PrimitivesInfo> primitives;
	uint32_t vertexIndexBufferID = -1;
	uint32_t entitiesSubscribed = -1;
	char path[MAX_PATH_LEN];
};