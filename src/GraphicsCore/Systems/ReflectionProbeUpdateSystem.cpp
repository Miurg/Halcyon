#include "GraphicsCore/Systems/ReflectionProbeUpdateSystem.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/GIBaker/ReflectionProbeBaker.hpp"
#include <iostream>

void ReflectionProbeUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "ReflectionProbeUpdateSystem registered!" << std::endl;
}

void ReflectionProbeUpdateSystem::onShutdown(GeneralManager& gm) 
{
	std::cout << "ReflectionProbeUpdateSystem shutdown!" << std::endl;
}

void ReflectionProbeUpdateSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	ReflectionProbeComponent* probe = gm.getComponent<ReflectionProbeComponent>(entity);
	if (probe) _agents.push_back({entity, probe});
}

void ReflectionProbeUpdateSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	for (const Agent& a : _agents)
		if (a.entity == entity && a.probe->cubemapIndex >= 0)
			_slotUsed[a.probe->cubemapIndex] = false;

	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& a) { return a.entity == entity; });
	_agents.erase(it, _agents.end());
}

// Reuses the probe's existing slot on rebake, otherwise claims the first free one; -1 when full.
int ReflectionProbeUpdateSystem::acquireSlot(ReflectionProbeComponent* probe)
{
	if (probe->cubemapIndex >= 0) return probe->cubemapIndex;
	for (int s = 0; s < static_cast<int>(MAX_REFLECTION_PROBES); ++s)
		if (!_slotUsed[s])
		{
			_slotUsed[s] = true;
			return s;
		}
	return -1;
}

void ReflectionProbeUpdateSystem::update(GeneralManager& gm)
{
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t currentFrame = currentFrameComp->currentFrame;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();

	// Bake any dirty probes first
	for (const Agent& a : _agents)
	{
		if (!a.probe->needBake) continue;
		int slot = acquireSlot(a.probe);
		if (slot < 0) continue; // reflection array full
		ReflectionProbeBaker::bake(gm, *a.probe, slot);
		a.probe->needBake = false;
	}

	auto* probeData = static_cast<ReflectionProbeData*>(
	    bufferManager.buffers[globalDSetComponent->reflectionProbeBuffer.id].bufferMapped[currentFrame]);

	uint32_t written = 0;
	for (const Agent& a : _agents)
	{
		// Only baked probes carry a valid cubemap slot; skip the rest so the shader never samples an unbound one.
		if (a.probe->cubemapIndex < 0 || written >= MAX_REFLECTION_PROBES) continue;

		probeData[written].boxMin = a.probe->origin - a.probe->halfExtent;
		probeData[written].boxMax = a.probe->origin + a.probe->halfExtent;
		probeData[written].captureOrigin = a.probe->origin;
		probeData[written].cubemapIndex = static_cast<uint32_t>(a.probe->cubemapIndex);
		++written;
	}

	*static_cast<uint32_t*>(
	    bufferManager.buffers[globalDSetComponent->reflectionProbeCountBuffer.id].bufferMapped[currentFrame]) = written;
}
