#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <cstdint>

struct HALCYON_API ImageDesc
{
	uint32_t width = 0;
	uint32_t height = 0;
	vk::Format format = vk::Format::eUndefined;
	vk::ImageUsageFlags usage;
	uint32_t mipLevels = 1;
	uint32_t layerCount = 1;
	vk::ImageCreateFlags flags = {};
	vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
	vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
	VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
};

namespace imagePresets
{
	// Cube images must be square, so a single size covers both dimensions.
	inline ImageDesc cubemap(uint32_t size, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels = 1)
	{
		ImageDesc desc;
		desc.width = size;
		desc.height = size;
		desc.format = format;
		desc.usage = usage;
		desc.mipLevels = mipLevels;
		desc.layerCount = 6;
		desc.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		return desc;
	}
}
