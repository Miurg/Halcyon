#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"

class PipelineFactory
{
public:
	static void createGraphicsPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
	                                   DescriptorManager& descriptorManager,
	                                   PipelineHandler& pipelineHandler);
	static void createShadowPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
	                                 DescriptorManager& descriptorManager,
	                                 PipelineHandler& pipelineHandler);
	[[nodiscard]] static vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,
	                                                               VulkanDevice& vulkanDevice);
};