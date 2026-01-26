#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"

class LoadFileFactory
{
public:
	static std::tuple<PrimitivesInfo, std::vector<unsigned char>, int, int>
	addMeshFromFile(const char path[MAX_PATH_LEN],
	                                                                                  VertexIndexBuffer& mesh);
	static void uploadTextureFromFile(const char* texturePath, Texture& texture, VmaAllocator& allocator,
	                                  VulkanDevice& vulkanDevice);
	static std::tuple<int, int> getSizesFromFileTexture(const char texturePath[MAX_PATH_LEN]);
	static void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
	                                  VulkanDevice& vulkanDevice);
	static void uploadTextureFromBuffer(const unsigned char* pixels, int texWidth, int texHeight,
	                                              Texture& texture, VmaAllocator& allocator, VulkanDevice& vulkanDevice);
};