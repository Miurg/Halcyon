#pragma once

#include "../VulkanDevice.hpp"

struct VMAllocatorComponent
{
	VmaAllocator allocator;

	VMAllocatorComponent(VmaAllocator allocator) : allocator(allocator) {}
};