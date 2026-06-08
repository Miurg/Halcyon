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

struct PushConstants
{
	uint frameNumber;
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
    if (dispatchThreadID.x >= particlesMetadataBuffer[0].maxNumberOfPatricles - particlesMetadataBuffer[0].numberOfParticles)
    {
        return;
    }

	int index;
	InterlockedAdd(particlesMetadataBuffer[0].bottomOfStack, 1, index);
    index = particlesStackBuffer[index];

    uint localSeed = dispatchThreadID.x + pushConstants.frameNumber * 719393u;
    particlesBuffer[index].seed = localSeed;

    float spawnPositionX = lerp(particlesMetadataBuffer[0].spawnRadius[0], particlesMetadataBuffer[0].spawnRadius[1],
                                random_float(localSeed));
    float spawnPositionY = lerp(particlesMetadataBuffer[0].spawnRadius[0], particlesMetadataBuffer[0].spawnRadius[1],
                                random_float(localSeed));
    float spawnPositionZ = lerp(particlesMetadataBuffer[0].spawnRadius[0], particlesMetadataBuffer[0].spawnRadius[1],
                                random_float(localSeed));
    float3 spawnPosition = float3(spawnPositionX, spawnPositionY, spawnPositionZ);

    particlesBuffer[index].position = spawnPosition;
    particlesBuffer[index].liveTime = 0;
    particlesBuffer[index].scale = float3(1.0, 1.0, 1.0);
}
