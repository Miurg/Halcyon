#pragma once

#include "GraphicsCore/VulkanDevice.hpp"

struct VMAllocatorComponent
{
	VmaAllocator allocator;

	VMAllocatorComponent(VmaAllocator allocator) : allocator(allocator) {}
};