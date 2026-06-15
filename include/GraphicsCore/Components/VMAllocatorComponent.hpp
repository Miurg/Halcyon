#pragma once

#include <vk_mem_alloc.h>
#include "GraphicsCore/VulkanDevice.hpp"

struct VMAllocatorComponent
{
	VmaAllocator allocator;

	VMAllocatorComponent(VmaAllocator allocator) : allocator(allocator) {}
};