#pragma once
#include "IPass.hpp"
#include "../Resources/Managers/ResourceHandles.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/AutoExposureSettingsComponent.hpp"

class ExposurePass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	void drawExposurePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManager& dManager,
	                      BufferManager& bManager, PipelineManager& pManager, SwapChain& swapChain, float deltaTime,
	                      AutoExposureSettingsComponent& aeSettings);
	DSetHandle _dSetExposure;
	DSetHandle _dSetMainColor;
	BufferHandle _histogramBuffer;
	BufferHandle _exposureBuffer;
};
