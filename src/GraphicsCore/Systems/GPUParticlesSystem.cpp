#include "GraphicsCore/Systems/GPUParticlesSystem.hpp"

#include "GraphicsCore/Components/ParticleEmitorComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/ParticlesBufferComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "../Passes/ParticleSystemComputePass.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"

void GPUParticlesSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "GPUParticlesSystem registered!" << std::endl;
}

void GPUParticlesSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "GPUParticlesSystem shutdown!" << std::endl;
}

void GPUParticlesSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("GPUParticlesSystem");
#endif

	ParticlesBufferComponent& dispatchSpawnBuffer =
	    *gm.getContextComponent<ParticlesBufferContext, ParticlesBufferComponent>();

	BufferManager& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	auto* dispatchSpawnBufferPtr = bufferManager.getMapped<DispatchIndirect>(dispatchSpawnBuffer.dispatchBuffer);
	auto* emitersDataBufferPtr = bufferManager.getMapped<EmiterData>(dispatchSpawnBuffer.emitersData);

	int spawnCountAll = 0;
	uint32_t emitterIndex = 0;
	forEachSubscribedEntity(
	    gm,
	    [&](Orhescyon::Entity, ParticleEmitorComponent& particleEmitor, GlobalTransformComponent& transform)
	    {
		    const uint32_t i = emitterIndex++;
		    if (particleEmitor.active)
		    {
			    particleEmitor.emissionAccumulator += particleEmitor.spawnCount * deltaTime;
			    uint32_t spawnCountParticle = static_cast<uint32_t>(particleEmitor.emissionAccumulator);
			    spawnCountAll += spawnCountParticle;
			    particleEmitor.emissionAccumulator -= static_cast<float>(spawnCountParticle);

			    emitersDataBufferPtr[i].active = particleEmitor.active;
			    emitersDataBufferPtr[i].directionalVector = particleEmitor.directionalVector;
			    emitersDataBufferPtr[i].initialPosition = transform.getGlobalPosition();
			    emitersDataBufferPtr[i].scale = particleEmitor.scale;
			    emitersDataBufferPtr[i].spawnRadius = particleEmitor.spawnRadius;
			    emitersDataBufferPtr[i].spawnCount = spawnCountParticle;
			    emitersDataBufferPtr[i].timeToLive = particleEmitor.timeToLive;
			    emitersDataBufferPtr[i].velocity = particleEmitor.velocity;
		    }
	    });
	dispatchSpawnBufferPtr->x = (spawnCountAll + 63) / 64;
	dispatchSpawnBufferPtr->spawnCount = spawnCountAll;
}