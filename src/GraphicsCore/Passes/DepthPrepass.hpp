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
struct BindlessTextureDSetComponent;
struct DrawInfoComponent;

class DepthPrepass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void onSettingsChanged(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void draw(vk::raii::CommandBuffer& cmd, uint32_t frame, SwapChain& swapChain, DescriptorManagerComponent& descriptorManager,
	          GlobalDSetComponent& globalDSetComponent, BufferManager& bufferManager, ModelDSetComponent& objectDSetComponent,
	          BindlessTextureDSetComponent& bindlessTextureDSetComponent, ModelManager& modelManager,
	          const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager);

	void buildPipelines(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples, bool rebuild);
	void declareStreams(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples);
};
