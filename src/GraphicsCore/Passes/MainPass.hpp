#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include <vulkan/vulkan_raii.hpp>

class SwapChain;
class BufferManager;
class ModelManager;
class PipelineManager;
struct DescriptorManagerComponent;
struct GlobalDSetComponent;
struct ModelDSetComponent;
struct DrawInfoComponent;
struct BindlessTextureDSetComponent;

class MainPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void onSettingsChanged(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void draw(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t frame,
	          BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& dManager,
	          GlobalDSetComponent& globalDSetComponent, BufferManager& bManager, ModelDSetComponent& objectDSetComponent,
	          ModelManager& mManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager, bool hasSkybox);

	void declareStreams(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples);
	void buildPipelines(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples, int gtaoEnabled, bool rebuild);
};
