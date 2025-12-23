#pragma once

#include <vulkan/vulkan_raii.hpp>

class VulkanDevice
{
public:
	vk::raii::Context context = vk::raii::Context();
	vk::raii::Instance instance = nullptr;
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device device = nullptr;
	uint32_t graphicsIndex = 0;
	uint32_t presentIndex = 0;
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::Queue presentQueue = nullptr;
	vk::raii::SurfaceKHR surface = nullptr;
	vk::raii::CommandPool commandPool = nullptr;
};