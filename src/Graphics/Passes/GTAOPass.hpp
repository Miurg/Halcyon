#pragma once
#include "IPass.hpp"
#include <vulkan/vulkan_raii.hpp>

class SwapChain;
class PipelineManager;
struct DSetHandle;
struct DescriptorManagerComponent;
struct GtaoSettingsComponent;

class GTAOPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	void drawGtao(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
	              DSetHandle& gtaoDSet, DSetHandle& globalDSet, const GtaoSettingsComponent& gtaoSettings,
	              PipelineManager& pManager);
	void drawBlur(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
	              DSetHandle& blurDSet, float dirX, float dirY, PipelineManager& pManager);
};
