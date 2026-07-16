#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <cstdint>

struct HALCYON_API Texture
{
	vk::Image textureImage;
	VmaAllocation textureImageAllocation;
	vk::ImageView textureImageView;
	SamplerHandle samplerHandle;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layerCount = 1;
	uint32_t mipLevels = 1;
	vk::Format format = vk::Format::eUndefined;
	vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
	vk::ImageUsageFlags usage;
	VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
	vk::ImageAspectFlags aspectFlags;
	vk::ImageCreateFlags imageCreateFlags = {};
};