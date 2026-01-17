#pragma once

#include <vulkan/vulkan_raii.hpp>

struct GlobalDSetComponent
{
	std::vector<vk::DescriptorSet> objectDSets;

	int buffer = -1;
};