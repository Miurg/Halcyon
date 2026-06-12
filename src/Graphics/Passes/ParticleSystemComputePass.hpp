#pragma once
#include "IPass.hpp"
#include "../Resources/Managers/ResourceHandles.hpp"
#include <glm/glm.hpp>

class DescriptorManagerComponent;
class BufferManager;
class PipelineManager;

struct alignas(16) DispatchIndirect
{
	alignas(4) uint32_t x;
	alignas(4) uint32_t y;
	alignas(4) uint32_t z;
	alignas(4) uint32_t spawnCount;
};

struct alignas(16) EmiterData
{
	alignas(4) bool active;
	alignas(4) uint32_t spawnCount;
	alignas(16) glm::vec3 initialPosition;
	alignas(16) glm::vec3 directionalVector;
	alignas(8) glm::vec2 spawnRadius; // 1-min, 2-max
	alignas(8) glm::vec2 timeToLive;  // 1-min, 2-max
	alignas(8) glm::vec2 velocity;    // 1-min, 2-max
	alignas(8) glm::vec2 scale;       // 1-min, 2-max
};

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
