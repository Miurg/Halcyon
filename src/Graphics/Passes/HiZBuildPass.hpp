#pragma once
#include "IPass.hpp"
#include <vulkan/vulkan_raii.hpp>

class PipelineManager;
struct DSetHandle;
struct DescriptorManagerComponent;

class HiZBuildPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle& dSetHandle,
	                    PipelineManager& pManager, uint32_t dstWidth, uint32_t dstHeight, uint32_t passIdx);
};
