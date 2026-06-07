// === Set 0: particleEmiterSet ===
struct Particle
{
	float3 position;
	float _padding;
	float3 scale;
	float timeToDie;
};

[vk::binding(0, 0)]
RWStructuredBuffer<Particle> particlesBuffer;

[vk::binding(1, 0)]
RWStructuredBuffer<int> particlesStackBuffer;

struct ParticleMetada
{
	float numberOfParticles;
    uint bottomOfStack;
    uint maxNumberOfPatricles;
	float spawnRadiusMax;
	float spawnRadiusMin;
	float timeToLiveMax;
	float timeToLiveMin;
};

[vk::binding(2, 0)]
RWStructuredBuffer<ParticleMetada> particlesMetadataBuffer;

struct VkDispatchIndirectCommand
{
	uint32_t x;
	uint32_t y;
    uint32_t z;
    uint32_t spawnCount;
};

[vk::binding(3, 0)]
RWStructuredBuffer<VkDispatchIndirectCommand> indirectBuffer;

struct VkDrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};
[[vk::binding(4, 0)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[vk::binding(5, 0)]
RWStructuredBuffer<uint> aliveIndicesBuffer;

struct PushConstants
{
	float deltaTime;
};

[[vk::push_constant]]
PushConstants pushConstants;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 dispatchThreadID: SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= particlesMetadataBuffer[0].maxNumberOfPatricles)
    {
        return;
    }

    if (particlesBuffer[dispatchThreadID.x].timeToDie <= 0.0)
    {
        return;
    }
    particlesBuffer[dispatchThreadID.x].timeToDie -= pushConstants.deltaTime;

    if (particlesBuffer[dispatchThreadID.x].timeToDie <= 0)
    {
        uint index;
        InterlockedAdd(particlesMetadataBuffer[0].bottomOfStack, -1, index);
        particlesStackBuffer[index - 1] = dispatchThreadID.x;

        uint currentSpawnIndex;
        InterlockedAdd(indirectBuffer[0].spawnCount, 1, currentSpawnIndex);

        if (currentSpawnIndex % 64 == 0)
        {
            InterlockedAdd(indirectBuffer[0].x, 1);
        }
    }
    else
    {
        uint drawIndex;
        InterlockedAdd(indirectDrawBuffer[0].instanceCount, 1, drawIndex);
        aliveIndicesBuffer[drawIndex] = dispatchThreadID.x;
    }

}