#pragma once
#include "IPass.hpp"
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
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void draw(vk::raii::CommandBuffer& cmd, uint32_t frame, SwapChain& swapChain,
	          DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
	          BufferManager& bManager, ModelDSetComponent& objectDSetComponent,
	          BindlessTextureDSetComponent& bindlessTextureDSetComponent, ModelManager& mManager,
	          const DrawInfoComponent& drawInfo, PipelineManager& pManager);
};
