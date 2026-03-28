#include "PipelineManager.hpp"
#include "../VulkanUtils.hpp"

PipelineManager::PipelineManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

PipelineManager::~PipelineManager()
{
	pipelines.clear();
}

void PipelineManager::build(vk::raii::Device& device, const PipelineDescription& desc)
{
	BuiltPipeline newpipeline = PipelineFactory::build(device, desc);

	std::string pipelineName = VulkanUtils::nameFromPath(desc.shaderPath);

	auto [it, inserted] = pipelines.try_emplace(pipelineName, std::move(newpipeline));
	if (!inserted)
	{
		// TODO: return some message
		return;
	}
}

void PipelineManager::build(vk::raii::Device& device, const PipelineDescription& desc, std::string pipelineName)
{
	BuiltPipeline newpipeline = PipelineFactory::build(device, desc);

	auto [it, inserted] = pipelines.try_emplace(pipelineName, std::move(newpipeline));
	if (!inserted)
	{
		// TODO: return some message
		return;
	}
}

void PipelineManager::rebuild(vk::raii::Device& device, const PipelineDescription& desc, std::string pipelineName)
{
	BuiltPipeline newpipeline = PipelineFactory::build(device, desc);
	pipelines[pipelineName] = std::move(newpipeline);
}