#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <cstddef>

class TextureManager;
class DescriptorManager;
struct BindlessTextureDSetComponent;
struct VulkanDevice;

// Composes TextureManager primitives (allocate/image/view/sampler) into common texture operations.
class HALCYON_API TextureFactory
{
public:
	static TextureHandle createDepthImage(TextureManager& tManager, uint32_t width, uint32_t height);
	static TextureHandle createOffscreenImage(TextureManager& tManager, uint32_t width, uint32_t height,
	                                          vk::Format format);
	static TextureHandle createShadowMap(TextureManager& tManager, uint32_t width, uint32_t height);
	static TextureHandle generateTextureData(TextureManager& tManager, VulkanDevice& vulkanDevice,
	                                         VmaAllocator allocator, const char* texturePath, int texWidth,
	                                         int texHeight, const unsigned char* pixels,
	                                         BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
	                                         vk::Format format = vk::Format::eR8G8B8A8Srgb);
	static TextureHandle generateTextureDataFromKtx(TextureManager& tManager, VulkanDevice& vulkanDevice,
	                                                VmaAllocator allocator, const char* texturePath,
	                                                const unsigned char* ktxData, size_t dataSize,
	                                                BindlessTextureDSetComponent& dSetComponent,
	                                                DescriptorManager& dManager, bool isSrgb);
};
