#pragma once
#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>

struct TextureData
{
	std::string name;
	std::vector<unsigned char> pixels;
	int width;
	int height;
};

struct LoadedPrimitive
{
	PrimitivesInfo info;
	std::shared_ptr<TextureData> texture;
};

class GltfLoader
{
public:
	static std::vector<LoadedPrimitive> loadMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh);
	static std::shared_ptr<TextureData> createDefaultWhiteTexture();
};


