#include "FrameManager.hpp"
#include "FrameManager.hpp"
#include "FrameManager.hpp"

int FrameManager::initFrameData()
{
	FrameData frameData;
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

	vk::CommandBufferAllocateInfo allocInfoSecondary;
	allocInfoSecondary.commandPool = vulkanDevice.commandPool;
	allocInfoSecondary.level = vk::CommandBufferLevel::eSecondary;
	allocInfoSecondary.commandBufferCount = 7; //shadow, cull, depth prepass, main, ssao, ssaoblur, fxaa

	vk::raii::CommandBuffers secondaryBuffers(vulkanDevice.device, allocInfoSecondary);
	frameData.secondaryCommandBuffers = std::move(secondaryBuffers);
	frames.push_back(std::move(frameData));
	return static_cast<int>(frames.size() - 1);
}

FrameManager::FrameManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

FrameManager::~FrameManager()
{
	if (!frames.empty())
	{
		for (auto& frame : frames)
		{
			frame.commandBuffer.clear();
			frame.renderFinishedSemaphore.clear();
			frame.presentCompleteSemaphore.clear();
			frame.inFlightFence.clear();
			frame.secondaryCommandBuffers.clear();
		}
	}
}
