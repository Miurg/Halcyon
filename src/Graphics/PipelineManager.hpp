#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "VulkanDevice.hpp"
#include "Model.hpp"
#include "SwapChain.hpp"
#include "DescriptorHandler.hpp"

class PipelineManager
{
public:
	PipelineManager(VulkanDevice& vulkanDevice, SwapChain& swapChain, DescriptorHandler& descriptorHandler);
	~PipelineManager();
	void createGraphicsPipeline();
	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;

private:
	VulkanDevice& vulkanDevice;
	SwapChain& swapChain;
	DescriptorHandler& descriptorHandler;
};
