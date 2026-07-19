#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

class SwapChain;
class PipelineManager;
struct DescriptorManagerComponent;
struct GtaoSettingsComponent;

class GTAOPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	void drawGtao(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& descriptorManager,
	              DSetHandle gtaoDSet, DSetHandle globalDSet, const GtaoSettingsComponent& gtaoSettings,
	              PipelineManager& pipelineManager);
	void drawBlur(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& descriptorManager,
	              DSetHandle blurDSet, float dirX, float dirY, const GtaoSettingsComponent& gtaoSettings,
	              PipelineManager& pipelineManager);

	DSetHandle _gtaoDset;
	DSetHandle _blurHDset;
	DSetHandle _blurVDset;
	TextureHandle _noiseTexture;
};
