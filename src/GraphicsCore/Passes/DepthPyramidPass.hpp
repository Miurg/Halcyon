#pragma once
#include "IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

class PipelineManager;
struct DescriptorManagerComponent;

class DepthPyramidPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	static constexpr uint32_t kMaxMips = 16;

	void drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle dSetHandle,
	                    DSetHandle globalDSet, PipelineManager& pManager, uint32_t dstWidth, uint32_t dstHeight,
	                    uint32_t passIdx, float edgeRange);

	DSetHandle _dsets[kMaxMips];
};
