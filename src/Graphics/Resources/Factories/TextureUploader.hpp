#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>

class TextureUploader
{
public:
	static void uploadTextureFromFile(const char* texturePath, Texture& texture, VmaAllocator& allocator,
	                                  VulkanDevice& vulkanDevice);
	static void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
	                                  VulkanDevice& vulkanDevice);
	static void uploadTextureFromBuffer(const unsigned char* pixels, int texWidth, int texHeight, Texture& texture,
	                                    VmaAllocator& allocator, VulkanDevice& vulkanDevice);
};