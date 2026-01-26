#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>

class GltfLoader
{
public:
	static std::tuple<PrimitivesInfo, std::vector<unsigned char>, int, int>
	loadMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh);
};