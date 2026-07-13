#include "GraphicsCore/Systems/ReflectionProbeUpdateSystem.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/GIBaker/ReflectionProbeBaker.hpp"
#include <iostream>
#include <vector>

void ReflectionProbeUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "ReflectionProbeUpdateSystem registered!" << std::endl;
}

void ReflectionProbeUpdateSystem::onShutdown(GeneralManager& gm) 
{
	std::cout << "ReflectionProbeUpdateSystem shutdown!" << std::endl;
}

void ReflectionProbeUpdateSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	// Fires before the entity is torn down, so the component is still readable here.
	ReflectionProbeComponent* probe = gm.getComponent<ReflectionProbeComponent>(entity);
	if (probe && probe->cubemapIndex >= 0) _slotUsed[probe->cubemapIndex] = false;
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
	std::vector<ReflectionProbeComponent*> dirtyProbes;
	forEachSubscribedEntity(gm,
	                        [&](Orhescyon::Entity, ReflectionProbeComponent& probe)
	                        {
		                        if (probe.needBake) dirtyProbes.push_back(&probe);
	                        });
	for (ReflectionProbeComponent* probe : dirtyProbes)
	{
		int slot = acquireSlot(probe);
		if (slot < 0) continue; // reflection array full
		ReflectionProbeBaker::bake(gm, *probe, slot);
		probe->needBake = false;
	}

	auto* probeData =
	    bufferManager.getMapped<ReflectionProbeData>(globalDSetComponent->reflectionProbeBuffer, currentFrame);

	uint32_t written = 0;
	forEachSubscribedEntity(
	    gm,
	    [&](Orhescyon::Entity, ReflectionProbeComponent& probe)
	    {
		    // Only baked probes carry a valid cubemap slot; skip the rest so the shader never samples an unbound one.
		    if (probe.cubemapIndex < 0 || written >= MAX_REFLECTION_PROBES) return;

		    probeData[written].boxMin = probe.origin - probe.halfExtent;
		    probeData[written].boxMax = probe.origin + probe.halfExtent;
		    probeData[written].captureOrigin = probe.origin;
		    probeData[written].cubemapIndex = static_cast<uint32_t>(probe.cubemapIndex);
		    ++written;
	    });

	*bufferManager.getMapped<uint32_t>(globalDSetComponent->reflectionProbeCountBuffer, currentFrame) = written;
}
