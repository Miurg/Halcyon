#pragma once

#include "HalcyonExport.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/VulkanDevice.hpp"

struct HALCYON_API VMAllocatorComponent
{
	VmaAllocator allocator;

	VMAllocatorComponent(VmaAllocator allocator) : allocator(allocator) {}
};