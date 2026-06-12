#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"

class TextureManager;

class TextureUploader
{
public:
	static void uploadTextureFromFile(const char* texturePath, Texture& texture, VmaAllocator& allocator,
	                                  VulkanDevice& vulkanDevice);
	static void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
	                                  VulkanDevice& vulkanDevice);
	static void uploadTextureFromBuffer(const unsigned char* pixels, int texWidth, int texHeight, Texture& texture,
	                                    VmaAllocator& allocator, VulkanDevice& vulkanDevice);
	static void uploadHdrTextureFromBuffer(const float* pixels, int texWidth, int texHeight, Texture& texture,
	                                       VmaAllocator& allocator, VulkanDevice& vulkanDevice);
	static void uploadHdrTextureFromFile(const char* texturePath, Texture& texture, TextureManager& tManager,
	                                     VmaAllocator& allocator, VulkanDevice& vulkanDevice);
	static void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
	                            uint32_t mipLevels, VulkanDevice& vulkanDevice);
	static void uploadKtxTextureData(const unsigned char* ktxData, size_t dataSize, Texture& texture, bool isSrgb,
	                                 VmaAllocator& allocator, VulkanDevice& vulkanDevice);
};