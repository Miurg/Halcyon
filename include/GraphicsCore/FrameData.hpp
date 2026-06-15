#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "GraphicsCore/VulkanDevice.hpp"

struct FrameData
{
	vk::raii::Semaphore presentCompleteSemaphore = nullptr;
	vk::raii::Fence inFlightFence = nullptr;
	vk::raii::CommandBuffer commandBuffer = nullptr;
};