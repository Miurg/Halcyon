#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

class DescriptorManagerComponent;
class BufferManager;
class PipelineManager;
class GlobalDSetComponent;

class ParticleSystemRenderPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;

private:
	void drawParticlRender(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
	                       BufferManager& bManager, PipelineManager& pManager, GlobalDSetComponent& globalDSetComponent,
	                       BufferHandle& indirectBuffer, uint32_t totalFrames);
	DSetHandle _dSetParticles;
};
