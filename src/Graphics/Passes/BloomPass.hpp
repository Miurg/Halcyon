#pragma once
#include "IPass.hpp"
#include <vulkan/vulkan_raii.hpp>

class PipelineManager;
struct DSetHandle;
struct DescriptorManagerComponent;

class BloomPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	void drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle& dSetHandle,
	                    PipelineManager& pManager, float texelSizeX, float texelSizeY, float threshold, float knee,
	                    int isFirstPass, vk::Extent2D extent);
	void drawUpsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle& dSetHandle,
	                  PipelineManager& pManager, float texelSizeX, float texelSizeY, float blendFactor,
	                  vk::Extent2D extent);
};
