#include "Shared/GpuStructs.h"
#include "Shared/ParticlesStructs.h"

// === Set 0: particleEmiterSet ===
[vk::binding(0, 0)]
RWStructuredBuffer<Particle> particlesBuffer;

[vk::binding(1, 0)]
RWStructuredBuffer<int> particlesStackBuffer;

[vk::binding(2, 0)]
RWStructuredBuffer<EmiterData> emitersDataBuffer;

[vk::binding(3, 0)]
RWStructuredBuffer<IndirectDispatchCommand> indirectBuffer;

[[vk::binding(4, 0)]]
RWStructuredBuffer<IndirectDrawCommand> indirectDrawBuffer;

[vk::binding(5, 0)]
RWStructuredBuffer<uint> aliveIndicesBufferA;

[vk::binding(6, 0)]
RWStructuredBuffer<uint> aliveIndicesBufferB;

[vk::binding(7, 0)]
RWStructuredBuffer<IndirectDispatchCommand> _dispatchBufferForEmiterA;

[vk::binding(8, 0)]
RWStructuredBuffer<IndirectDispatchCommand> _dispatchBufferForEmiterB;

[vk::binding(9, 0)]
RWStructuredBuffer<ParticlesMetadata> _particlesMetadata;

struct PushConstants
{
	uint totalFrame;
};

[[vk::push_constant]]
PushConstants pushConstants;

uint pcg_hash(inout uint seed)
{
	uint state = seed;
	seed = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float random_float(inout uint seed)
{
	return float(pcg_hash(seed)) / 4294967295.0;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 dispatchThreadID: SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= indirectBuffer[0].spawnCount)
    {
        return;
    }
    if (dispatchThreadID.x >= _particlesMetadata[0].maxNumberOfPatricles)
    {
        return;
    }

    // TODO: implement better lookup way
    uint tempSpawnCount = 0;
    uint currentEmitorIndex = 0;
    for (uint i = 0; i < _particlesMetadata[0].numberOfEmiters; ++i)
    {
        tempSpawnCount += emitersDataBuffer[i].spawnCount;
        if (tempSpawnCount > dispatchThreadID.x)
        {
            currentEmitorIndex = i;
            break;
        }
    }

    EmiterData currentEmitor = emitersDataBuffer[currentEmitorIndex];

    int index;
    InterlockedAdd(_particlesMetadata[0].bottomOfStack, 1, index);
    index = particlesStackBuffer[index];

    uint localSeed = dispatchThreadID.x + pushConstants.totalFrame * 719393u;

    float offset = (currentEmitor.spawnRadius[0] + currentEmitor.spawnRadius[1]) * 0.5;

    float spawnPositionX = lerp(currentEmitor.spawnRadius[0], currentEmitor.spawnRadius[1],
                                random_float(localSeed)) - offset;
    float spawnPositionY = lerp(currentEmitor.spawnRadius[0], currentEmitor.spawnRadius[1],
                                random_float(localSeed)) - offset;
    float spawnPositionZ = lerp(currentEmitor.spawnRadius[0], currentEmitor.spawnRadius[1],
                                random_float(localSeed)) - offset;
    float3 spawnPosition = float3(spawnPositionX, spawnPositionY, spawnPositionZ);

    particlesBuffer[index].position = currentEmitor.initialPosition + spawnPosition;
    particlesBuffer[index].liveTime = 0;
    particlesBuffer[index].scale = float3(1.0, 1.0, 1.0);
    particlesBuffer[index].seed = localSeed;
    particlesBuffer[index].emiterIndex = currentEmitorIndex;
    particlesBuffer[index].color = currentEmitor.colorStart;

    uint indexDispatch;
    if (pushConstants.totalFrame % 2 == 0)
    {
        InterlockedAdd(_dispatchBufferForEmiterA[0].spawnCount, 1, indexDispatch);
        if (indexDispatch % 64 == 0)
        {
            InterlockedAdd(_dispatchBufferForEmiterA[0].x, 1);
        }
        aliveIndicesBufferA[indexDispatch] = index;
    }
    else
    {
        InterlockedAdd(_dispatchBufferForEmiterB[0].spawnCount, 1, indexDispatch);
        if (indexDispatch % 64 == 0)
        {
            InterlockedAdd(_dispatchBufferForEmiterB[0].x, 1);
        }
        aliveIndicesBufferB[indexDispatch] = index;
    }

}
