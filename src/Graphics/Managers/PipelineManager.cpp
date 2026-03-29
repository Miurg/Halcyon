#include "PipelineManager.hpp"
#include "../VulkanUtils.hpp"

PipelineManager::PipelineManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

PipelineManager::~PipelineManager()
{
	pipelines.clear();
}

void PipelineManager::build(const PipelineDescription& desc)
{
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc);
	newpipeline.desc = desc;
	std::string pipelineName = VulkanUtils::nameFromPath(desc.shaderPath);

	auto [it, inserted] = pipelines.try_emplace(pipelineName, std::move(newpipeline));
	if (!inserted)
	{
		// TODO: return some message
		return;
	}
}

void PipelineManager::build(const PipelineDescription& desc, std::string pipelineName)
{
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc);
	newpipeline.desc = desc;
	auto [it, inserted] = pipelines.try_emplace(pipelineName, std::move(newpipeline));
	if (!inserted)
	{
		// TODO: return some message
		return;
	}
}

void PipelineManager::rebuild(std::string pipelineName)
{
	PipelineDescription desc = pipelines[pipelineName].desc;
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc);
	newpipeline.desc = desc;
	pipelines[pipelineName] = std::move(newpipeline);
}

void PipelineManager::rebuild(const PipelineDescription& desc, std::string pipelineName)
{
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc);
	newpipeline.desc = desc;
	pipelines[pipelineName] = std::move(newpipeline);
}