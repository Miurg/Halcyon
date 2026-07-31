#pragma once
#include "GraphicsCore/Passes/IPass.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "Shared/GpuStructs.h"
#include <glm/glm.hpp>

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
	void drawParticleCompute(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
	                         BufferManager& bufferManager, PipelineManager& pipelineManager, uint32_t totalFrames, float deltaTime);
	DSetHandle _dSetParticles;
	BufferHandle _emitersData;
	BufferHandle _particlesStack;
	BufferHandle _dispatchBuffer;
	BufferHandle _indirectBuffer;
	BufferHandle _aliveIndicesBufferA;
	BufferHandle _aliveIndicesBufferB;
	BufferHandle _dispatchBufferForEmiterA;
	BufferHandle _dispatchBufferForEmiterB;
	BufferHandle _particlesMetadata;
};
