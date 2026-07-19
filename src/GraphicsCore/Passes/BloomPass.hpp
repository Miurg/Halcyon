#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

class PipelineManager;
struct DescriptorManagerComponent;

class BloomPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	static constexpr uint32_t kMipCount = 5;

	void drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& descriptorManager, DSetHandle dSetHandle,
	                    PipelineManager& pipelineManager, float texelSizeX, float texelSizeY, float threshold, float knee,
	                    int isFirstPass, vk::Extent2D extent);
	void drawUpsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& descriptorManager, DSetHandle dSetHandle,
	                  PipelineManager& pipelineManager, float texelSizeX, float texelSizeY, float blendFactor,
	                  vk::Extent2D extent);

	DSetHandle _downsampleDsets[kMipCount];
	DSetHandle _upsampleDsets[kMipCount];
};
