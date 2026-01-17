#pragma once

#include <vulkan/vulkan_raii.hpp>

struct ObjectDSetComponent
{
	vk::DescriptorSet StorageBufferDSet[MAX_FRAMES_IN_FLIGHT];

	int StorageBuffer = -1;
};