#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/ImageDesc.hpp"
#include "GraphicsCore/SamplerDesc.hpp"
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
	static TextureHandle createTexture(TextureManager& textureManager, const ImageDesc& desc,
	                                   const SamplerDesc& samplerDesc,
	                                   vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor,
	                                   vk::ImageViewType viewType = vk::ImageViewType::e2D);
	static TextureHandle createDepthImage(TextureManager& textureManager, uint32_t width, uint32_t height);
	static TextureHandle createOffscreenImage(TextureManager& textureManager, uint32_t width, uint32_t height,
	                                          vk::Format format);
	static TextureHandle createShadowMap(TextureManager& textureManager, uint32_t width, uint32_t height);
	static TextureHandle createBindlessTexture(TextureManager& textureManager, VulkanDevice& vulkanDevice,
	                                           VmaAllocator allocator, const char* texturePath, int texWidth,
	                                           int texHeight, const unsigned char* pixels,
	                                           BindlessTextureDSetComponent& dSetComponent,
	                                           DescriptorManager& descriptorManager,
	                                           vk::Format format = vk::Format::eR8G8B8A8Srgb);
	static TextureHandle createBindlessTextureFromKtx(TextureManager& textureManager, VulkanDevice& vulkanDevice,
	                                                  VmaAllocator allocator, const char* texturePath,
	                                                  const unsigned char* ktxData, size_t dataSize,
	                                                  BindlessTextureDSetComponent& dSetComponent,
	                                                  DescriptorManager& descriptorManager, bool isSrgb);
};
