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
    int bottomOfStack;
    int maxNumberOfPatricles;
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

	float timeToDie = lerp(particlesMetadataBuffer[0].timeToLiveMin, particlesMetadataBuffer[0].timeToLiveMax,
	                       random_float(localSeed));

    float spawnPositionX = lerp(particlesMetadataBuffer[0].spawnRadiusMin, particlesMetadataBuffer[0].spawnRadiusMax,
                                random_float(localSeed));
    float spawnPositionY = lerp(particlesMetadataBuffer[0].spawnRadiusMin, particlesMetadataBuffer[0].spawnRadiusMax,
                                random_float(localSeed));
    float spawnPositionZ = lerp(particlesMetadataBuffer[0].spawnRadiusMin, particlesMetadataBuffer[0].spawnRadiusMax,
                                random_float(localSeed));
    float3 spawnPosition = float3(spawnPositionX, spawnPositionY, spawnPositionZ);
    particlesBuffer[index].position = spawnPosition;
    particlesBuffer[index].timeToDie = timeToDie;
    particlesBuffer[index].scale = float3(0.1, 0.1, 0.1);
}
