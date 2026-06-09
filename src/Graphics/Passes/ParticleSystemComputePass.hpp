#pragma once
#include "IPass.hpp"
#include "../Resources/Managers/ResourceHandles.hpp"

class DescriptorManagerComponent;
class BufferManager;
class PipelineManager;


class ParticleSystemComputePass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	BufferHandle _particlesBuffer;

private:
	void drawParticleCompute(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
	                         BufferManager& bManager, PipelineManager& pManager, uint32_t totalFrames, float deltaTime);
	DSetHandle _dSetParticles;
	BufferHandle _particlesMetada;
	BufferHandle _particlesStack;
	BufferHandle _dispatchBuffer;
	BufferHandle _indirectBuffer;
	BufferHandle _aliveIndicesBufferA;
	BufferHandle _aliveIndicesBufferB;
	BufferHandle _dispatchBufferForEmiterA;
	BufferHandle _dispatchBufferForEmiterB;
};
