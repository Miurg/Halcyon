#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../../Platform/Window.hpp"
#include "../VulkanDevice.hpp"


class VulkanDeviceFactory
{
public:
	static void createVulkanDevice(Window* window, VulkanDevice* vulkanDevice);
	static void createInstance(Window* window, VulkanDevice* vulkanDevice);
	static void createSurface(Window* window, VulkanDevice* vulkanDevice);
	static void pickPhysicalDevice(VulkanDevice* vulkanDevice);
	static void createLogicalDevice(VulkanDevice* vulkanDevice);
	static void createCommandPool(VulkanDevice* vulkanDevice);
};