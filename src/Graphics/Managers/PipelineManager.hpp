#pragma once

#include <vector>
#include "../Factories/PipelineFactory.hpp"
#include "../VulkanDevice.hpp"

class PipelineManager
{
public:
	PipelineManager(VulkanDevice& vulkanDevice);
	~PipelineManager();
	std::unordered_map<std::string, BuiltPipeline> pipelines;
	void build(vk::raii::Device& device, const PipelineDescription& desc);
	void build(vk::raii::Device& device, const PipelineDescription& desc, std::string pipelineName);

	void rebuild(vk::raii::Device& device, const PipelineDescription& desc, std::string pipelineName);

private:
	VulkanDevice& vulkanDevice;
};