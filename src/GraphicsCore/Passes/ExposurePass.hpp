#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/AutoExposureSettingsComponent.hpp"

class ExposurePass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	void drawExposurePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManager& descriptorManager,
	                      BufferManager& bufferManager, PipelineManager& pipelineManager, SwapChain& swapChain, float deltaTime,
	                      AutoExposureSettingsComponent& aeSettings);
	DSetHandle _dSetExposure;
	DSetHandle _dSetMainColor;
	BufferHandle _histogramBuffer;
	BufferHandle _exposureBuffer;
};
