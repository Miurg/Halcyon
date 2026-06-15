#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <vk_mem_alloc.h>

struct HALCYON_API Buffer
{
	std::vector<vk::Buffer> buffer;
	std::vector<VmaAllocation> bufferAllocation;
	std::vector<void*> bufferMapped;
};