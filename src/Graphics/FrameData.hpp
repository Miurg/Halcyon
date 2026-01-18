#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "VulkanDevice.hpp"

struct FrameData
{
	vk::raii::Semaphore presentCompleteSemaphore = nullptr;
	vk::raii::Semaphore renderFinishedSemaphore = nullptr;
	vk::raii::Fence inFlightFence = nullptr;
	vk::raii::CommandBuffer commandBuffer = nullptr;

	static void initFrameData(FrameData& frameData, VulkanDevice& vulkanDevice)
	{
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandPool = vulkanDevice.commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = 1;

		vk::raii::CommandBuffers buffers(vulkanDevice.device, allocInfo);
		frameData.commandBuffer = std::move(buffers[0]);
		frameData.renderFinishedSemaphore = vk::raii::Semaphore(vulkanDevice.device, vk::SemaphoreCreateInfo());
		frameData.presentCompleteSemaphore = vk::raii::Semaphore(vulkanDevice.device, vk::SemaphoreCreateInfo());
		frameData.inFlightFence =
		    vk::raii::Fence(vulkanDevice.device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	};
};