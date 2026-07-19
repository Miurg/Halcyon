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
	          BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& descriptorManager,
	          GlobalDSetComponent& globalDSetComponent, BufferManager& bufferManager, ModelDSetComponent& objectDSetComponent,
	          ModelManager& modelManager, const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager, bool hasSkybox);

	void declareStreams(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples);
	void buildPipelines(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples, int gtaoEnabled, bool rebuild);
};
