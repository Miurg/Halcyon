// === Set 0: particleEmiterSet ===
struct Particle
{
	float3 position;
	uint seed;
	float3 scale;
	float liveTime;
};

[vk::binding(0, 0)]
RWStructuredBuffer<Particle> particlesBuffer;

[vk::binding(1, 0)]
RWStructuredBuffer<int> particlesStackBuffer;

struct ParticleMetada
{
	float3 directionalVector;
	float numberOfParticles;
	uint bottomOfStack;
	uint maxNumberOfPatricles;
	float2 spawnRadius;
	float2 timeToLive;
	float2 velocity;
	float2 scale;
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
	if (dispatchThreadID.x >= particlesMetadataBuffer[0].maxNumberOfPatricles)
	{
		return;
	}

	if (particlesBuffer[dispatchThreadID.x].liveTime < 0.0) return;

	uint localSeed = particlesBuffer[dispatchThreadID.x].seed;
	float timeToDie = lerp(particlesMetadataBuffer[0].timeToLive[0], particlesMetadataBuffer[0].timeToLive[1],
	                       random_float(localSeed));

	if (particlesBuffer[dispatchThreadID.x].liveTime >= timeToDie)
	{
		return;
	}
	particlesBuffer[dispatchThreadID.x].liveTime += pushConstants.deltaTime;

	if (particlesBuffer[dispatchThreadID.x].liveTime >= timeToDie)
	{
		uint index;
		InterlockedAdd(particlesMetadataBuffer[0].bottomOfStack, -1, index);
		particlesStackBuffer[index - 1] = dispatchThreadID.x;

		particlesBuffer[dispatchThreadID.x].liveTime = -1.0;

		uint currentSpawnIndex;
		InterlockedAdd(indirectBuffer[0].spawnCount, 1, currentSpawnIndex);

		if (currentSpawnIndex % 64 == 0)
		{
			InterlockedAdd(indirectBuffer[0].x, 1);
		}
	}
	else
	{
		float velocity =
		    lerp(particlesMetadataBuffer[0].velocity[0], particlesMetadataBuffer[0].velocity[1], random_float(localSeed));

		particlesBuffer[dispatchThreadID.x].position =
		    particlesBuffer[dispatchThreadID.x].position +
		    particlesMetadataBuffer[0].directionalVector * velocity * pushConstants.deltaTime;

		float scale =
		    lerp(particlesMetadataBuffer[0].scale[0], particlesMetadataBuffer[0].scale[1], random_float(localSeed));

        particlesBuffer[dispatchThreadID.x].scale = float3(scale, scale, scale);

		uint drawIndex;
		InterlockedAdd(indirectDrawBuffer[0].instanceCount, 1, drawIndex);
		aliveIndicesBuffer[drawIndex] = dispatchThreadID.x;
	}
}
