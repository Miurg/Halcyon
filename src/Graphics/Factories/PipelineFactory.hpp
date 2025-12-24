#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../DescriptorHandler.hpp"
#include "../PipelineHandler.hpp"

class PipelineFactory
{
public:
	static void createGraphicsPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain, DescriptorHandler& descriptorHandler,
	                            PipelineHandler& pipelineHandler);
	[[nodiscard]] static vk::raii::ShaderModule createShaderModule(const std::vector<char>& code,
	                                                               VulkanDevice& vulkanDevice);
};