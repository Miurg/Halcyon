#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../Resources/Managers/BufferManager.hpp"

class PipelineFactory
{
public:
	static void createGraphicsPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain, BufferManager& bufferManager,
	                            PipelineHandler& pipelineHandler);
	[[nodiscard]] static vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,
	                                                               VulkanDevice& vulkanDevice);
};