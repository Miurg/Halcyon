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
	void build(const PipelineDescription& desc);
	void build(const PipelineDescription& desc, std::string pipelineName);

	void rebuild(std::string pipelineName);

	void rebuild(const PipelineDescription& desc, std::string pipelineName);

private:
	VulkanDevice& vulkanDevice;
};