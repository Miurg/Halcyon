#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../VulkanDevice.hpp"
#include "DescriptorManager.hpp"
#include "ResourceHandles.hpp"

class TextureManager
{
public:
	TextureManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~TextureManager();
	TextureHandle generateTextureData(const char texturePath[MAX_PATH_LEN], int texWidth, int texHeight,
	                                  const unsigned char* pixels, BindlessTextureDSetComponent& dSetComponent,
	                                  DescriptorManager& dManager);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	TextureHandle createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY);
	void createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void createTextureSampler(Texture& texture);
	void createShadowSampler(Texture& texture);
	vk::Format findBestFormat();
	vk::Format findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
	                                   vk::FormatFeatureFlags features);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture);
	std::vector<Texture> textures;
	std::unordered_map<std::string, TextureHandle> texturePaths;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
};