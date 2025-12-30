#pragma once

#include <vulkan/vulkan_raii.hpp>

struct SSBOs
{
	std::vector<vk::raii::Buffer> modelSSBOs;
	std::vector<vk::raii::DeviceMemory> modelSSBOsMemory;
	std::vector<void*> modelSSBOsMapped;
	std::vector<vk::raii::DescriptorSet> modelSSBOsDescriptorSets;
};