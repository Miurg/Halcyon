#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "VulkanDevice.hpp"

struct FrameData
{
	vk::raii::Semaphore presentCompleteSemaphore = nullptr;
	vk::raii::Semaphore renderFinishedSemaphore = nullptr;
	vk::raii::Fence inFlightFence = nullptr;
	vk::raii::CommandBuffer commandBuffer = nullptr;
	vk::raii::CommandBuffers secondaryCommandBuffers = nullptr;
};