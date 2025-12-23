#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../VulkanDevice.hpp"
#include "../DescriptorHandler.hpp"

class DescriptorHandlerFactory
{
public:
	static void createDescriptorSetLayout(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void createDescriptorPool(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler);
	static void allocateDescriptorSets(VulkanDevice& vulkanDevice, DescriptorHandler& descriptorHandler,
	                                   GameObject& gameObject);
	static void updateUniformDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject);
	static void updateTextureDescriptors(VulkanDevice& vulkanDevice, GameObject& gameObject);
};