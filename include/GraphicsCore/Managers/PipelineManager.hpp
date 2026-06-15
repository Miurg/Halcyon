#pragma once

#include <vector>
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/VulkanDevice.hpp"

class DescriptorManager;

class PipelineManager
{
public:
	PipelineManager(VulkanDevice& vulkanDevice, DescriptorManager& descriptorManager);
	~PipelineManager();
	std::unordered_map<std::string, BuiltPipeline> pipelines;
	void build(const PipelineDescription& desc);
	void build(const PipelineDescription& desc, std::string pipelineName);

	void rebuild(std::string pipelineName);

	void rebuild(const PipelineDescription& desc, std::string pipelineName);

private:
	VulkanDevice& vulkanDevice;
	DescriptorManager& descriptorManager;

	// Resolves desc.setLayoutNames to raw vk::DescriptorSetLayout handles via DescriptorManager
	std::vector<vk::DescriptorSetLayout> resolveLayouts(const PipelineDescription& desc) const;
};