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

void GPUParticlesSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	ParticleEmitorComponent* particleEmitor = gm.getComponent<ParticleEmitorComponent>(entity);
	GlobalTransformComponent* transform = gm.getComponent<GlobalTransformComponent>(entity);

	if (particleEmitor && transform)
	{
		_agents.push_back({entity, particleEmitor, transform});
	}
}

void GPUParticlesSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void GPUParticlesSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("GPUParticlesSystem");
#endif

	ParticlesBufferComponent& dispatchSpawnBuffer =
	    *gm.getContextComponent<ParticlesBufferContext, ParticlesBufferComponent>();

	BufferManager& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	auto* dispatchSpawnBufferPtr = static_cast<DispatchIndirect*>(bManager.buffers[dispatchSpawnBuffer.dispatchBuffer.id].bufferMapped[0]);
	auto* emitersDataBufferPtr =
	    static_cast<EmiterData*>(bManager.buffers[dispatchSpawnBuffer.emitersData.id].bufferMapped[0]);

	int spawnCountAll = 0;
	for (int i = 0; i < _agents.size(); ++i)
	{
		const auto& agent = _agents[i];
		if (agent.particleEmitor->active)
		{
			agent.particleEmitor->emissionAccumulator += agent.particleEmitor->spawnCount * deltaTime;
			uint32_t spawnCountParticle = static_cast<uint32_t>(agent.particleEmitor->emissionAccumulator);
			spawnCountAll += spawnCountParticle;
			agent.particleEmitor->emissionAccumulator -= static_cast<float>(spawnCountParticle);

			emitersDataBufferPtr[i].active = agent.particleEmitor->active;
			emitersDataBufferPtr[i].directionalVector = agent.particleEmitor->directionalVector;
			emitersDataBufferPtr[i].initialPosition = agent.transform->getGlobalPosition();
			emitersDataBufferPtr[i].scale = agent.particleEmitor->scale;
			emitersDataBufferPtr[i].spawnRadius = agent.particleEmitor->spawnRadius;
			emitersDataBufferPtr[i].spawnCount = spawnCountParticle;
			emitersDataBufferPtr[i].timeToLive = agent.particleEmitor->timeToLive;
			emitersDataBufferPtr[i].velocity = agent.particleEmitor->velocity;
		}
	}
	dispatchSpawnBufferPtr->x = (spawnCountAll+63)/64;
	dispatchSpawnBufferPtr->spawnCount = spawnCountAll;
}