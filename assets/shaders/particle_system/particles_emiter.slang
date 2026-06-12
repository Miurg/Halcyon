// === Set 0: particleEmiterSet ===
struct Particle
{
	float3 position;
	uint seed;
	float3 scale;
    float liveTime;
    float3 rotation;
    uint emiterIndex;
};

[vk::binding(0, 0)]
RWStructuredBuffer<Particle> particlesBuffer;

[vk::binding(1, 0)]
RWStructuredBuffer<int> particlesStackBuffer;

struct EmiterData
{
    bool active;
    uint spawnCount;
    float3 initialPosition;
    float3 directionalVector;
	float2 spawnRadius;
	float2 timeToLive;
	float2 velocity;
	float2 scale;
};

[vk::binding(2, 0)]
RWStructuredBuffer<EmiterData> emitersDataBuffer;

struct VkDispatchIndirectCommand
{
    uint x;
    uint y;
    uint z;
    uint spawnCount;
};
[vk::binding(3, 0)]
RWStructuredBuffer<VkDispatchIndirectCommand> indirectBuffer;

struct VkDrawIndexedIndirectCommand
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};
[[vk::binding(4, 0)]]
RWStructuredBuffer<VkDrawIndexedIndirectCommand> indirectDrawBuffer;

[vk::binding(5, 0)]
RWStructuredBuffer<uint> aliveIndicesBufferA;

[vk::binding(6, 0)]
RWStructuredBuffer<uint> aliveIndicesBufferB;

[vk::binding(7, 0)]
RWStructuredBuffer<VkDispatchIndirectCommand> _dispatchBufferForEmiterA;

[vk::binding(8, 0)]
RWStructuredBuffer<VkDispatchIndirectCommand> _dispatchBufferForEmiterB;

struct ParticlesMetadata
{
    uint bottomOfStack;
    uint maxNumberOfPatricles;
    uint numberOfEmiters;
};
[vk::binding(9, 0)]
RWStructuredBuffer<ParticlesMetadata> _particlesMetadata;

struct PushConstants
{
    float deltaTime;
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
    uint currentSpawnCount;
    if (pushConstants.totalFrame % 2 == 0)
    {
        currentSpawnCount = _dispatchBufferForEmiterA[0].spawnCount;
    }
    else
    {
        currentSpawnCount = _dispatchBufferForEmiterB[0].spawnCount;
    }

    if (dispatchThreadID.x >= currentSpawnCount)
	{
		return;
    }

    uint index;
    if (pushConstants.totalFrame % 2 == 0)
    {
        index = aliveIndicesBufferA[dispatchThreadID.x];
    }
    else
    {
        index = aliveIndicesBufferB[dispatchThreadID.x];
    }

    if (particlesBuffer[index].liveTime < 0.0) return;

    uint emiterIndex = particlesBuffer[index].emiterIndex;
    EmiterData emiterBuffer = emitersDataBuffer[emiterIndex];

    uint localSeed = particlesBuffer[index].seed;
    float timeToDie = lerp(emiterBuffer.timeToLive[0], emiterBuffer.timeToLive[1],
	                       random_float(localSeed));

	if (particlesBuffer[index].liveTime >= timeToDie)
	{
		return;
	}
	particlesBuffer[index].liveTime += pushConstants.deltaTime;

	if (particlesBuffer[index].liveTime >= timeToDie)
    {
        uint stackIndex;
        InterlockedAdd(_particlesMetadata[0].bottomOfStack, -1, stackIndex);
        particlesStackBuffer[stackIndex - 1] = index;
		particlesBuffer[index].liveTime = -1.0;
	}
	else
	{
        float velocity =
		    lerp(emiterBuffer.velocity[0], emiterBuffer.velocity[1], random_float(localSeed));

        particlesBuffer[index].position =
            particlesBuffer[index].position +
		    emiterBuffer.directionalVector * velocity * pushConstants.deltaTime;

        float scale =
		    lerp(emiterBuffer.scale[0], emiterBuffer.scale[1], random_float(localSeed));

        particlesBuffer[index].scale = float3(scale, scale, scale);

        InterlockedAdd(indirectDrawBuffer[0].instanceCount, 1);

        uint indexDispatch;
        if (pushConstants.totalFrame % 2 == 0)
        { 
			InterlockedAdd(_dispatchBufferForEmiterB[0].spawnCount, 1, indexDispatch);
            if (indexDispatch % 64 == 0)
            {
                InterlockedAdd(_dispatchBufferForEmiterB[0].x, 1);
            }
            aliveIndicesBufferB[indexDispatch] = index;
        }
        else
        {
            InterlockedAdd(_dispatchBufferForEmiterA[0].spawnCount, 1, indexDispatch);
            if (indexDispatch % 64 == 0)
            {
                InterlockedAdd(_dispatchBufferForEmiterA[0].x, 1);
            }
            aliveIndicesBufferA[indexDispatch] = index;
           
        }
	}
}
