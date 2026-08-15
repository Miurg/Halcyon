#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

class PipelineManager;
struct DescriptorManagerComponent;

class ClusteredComputePass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void computeClustered(vk::raii::CommandBuffer& cmd, uint32_t frame,
	                      DescriptorManagerComponent& descriptorManager, DSetHandle globalDSet,
	                      PipelineManager& pipelineManager, uint32_t widthScreen, uint32_t heightScreen,
	                      vk::Buffer clusteredGridBuffer, vk::Buffer clusteredInfoBuffer,
	                      vk::Buffer visiblePointLightIndicesBuffer);
};
