#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "PlatformCore/Window.hpp"
#include "GraphicsCore/VulkanDevice.hpp"

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#else
using TracyVkCtx = void*;
#endif

class VulkanDeviceFactory
{
public:
	static void createVulkanDevice(Window& window, VulkanDevice& vulkanDevice);
	static void createInstance(Window& window, VulkanDevice& vulkanDevice);
	static void createSurface(Window& window, VulkanDevice& vulkanDevice);
	static void pickPhysicalDevice(VulkanDevice& vulkanDevice);
	static void createLogicalDevice(VulkanDevice& vulkanDevice);
	static void createCommandPool(VulkanDevice& vulkanDevice);
	static TracyVkCtx createTracyContext(const VulkanDevice& vulkanDevice);
};
