#pragma once

#include "../VulkanDevice.hpp"
#include "../FrameData.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../GameObject.hpp"
#include "../Model.hpp"
#include "../VulkanConst.hpp"
#include "../../Platform/Window.hpp"

class RenderSystem
{
public:
	RenderSystem(VulkanDevice& deviceContext, SwapChain& swapChain, PipelineHandler& pipelineHandler, Model& model,
	             Window& window);
	~RenderSystem();
	void drawFrame(std::array<GameObject, MAX_OBJECTS>& gameObjects);

private:
	void recordCommandBuffer(vk::raii::CommandBuffer* commandBuffer, uint32_t imageIndex,
	                         std::array<GameObject, MAX_OBJECTS>& gameObjects);
	void transitionImageLayout(vk::raii::CommandBuffer* commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                           vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
	                           vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
	                           vk::ImageAspectFlags imageAspectFlags);
	void updateUniformBuffer(uint32_t currentImage, std::array<GameObject, MAX_OBJECTS>& gameObjects);
	VulkanDevice& vulkanDevice;
	SwapChain& swapChain;
	PipelineHandler& pipelineHandler;
	Window& window;
	Model& model;
	std::vector<FrameData> framesData;
	uint32_t currentFrame = 0;
};