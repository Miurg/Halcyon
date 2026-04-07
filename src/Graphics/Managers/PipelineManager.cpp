#include "PipelineManager.hpp"
#include "../VulkanUtils.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"

PipelineManager::PipelineManager(VulkanDevice& vulkanDevice, DescriptorManager& descriptorManager)
    : vulkanDevice(vulkanDevice), descriptorManager(descriptorManager)
{
}

PipelineManager::~PipelineManager()
{
	pipelines.clear();
}

std::vector<vk::DescriptorSetLayout> PipelineManager::resolveLayouts(const PipelineDescription& desc) const
{
	std::vector<vk::DescriptorSetLayout> result;
	result.reserve(desc.setLayoutNames.size());
	for (const auto& name : desc.setLayoutNames)
		result.push_back(descriptorManager.getLayout(name));
	return result;
}

void PipelineManager::build(const PipelineDescription& desc)
{
	auto layouts = resolveLayouts(desc);
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc, layouts);
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
	auto layouts = resolveLayouts(desc);
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc, layouts);
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
	auto layouts = resolveLayouts(desc);
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc, layouts);
	newpipeline.desc = desc;
	pipelines[pipelineName] = std::move(newpipeline);
}

void PipelineManager::rebuild(const PipelineDescription& desc, std::string pipelineName)
{
	auto layouts = resolveLayouts(desc);
	BuiltPipeline newpipeline = PipelineFactory::build(vulkanDevice.device, desc, layouts);
	newpipeline.desc = desc;
	pipelines[pipelineName] = std::move(newpipeline);
}