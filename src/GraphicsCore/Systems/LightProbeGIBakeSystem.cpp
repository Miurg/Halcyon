#include "GraphicsCore/Systems/LightProbeGIBakeSystem.hpp"
#include <iostream>
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include "GraphicsCore/GIBaker/LightProbeGIBaking.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void LightProbeGIBakeSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "LightProbeGIBakeSystem registered!" << std::endl;
}

void LightProbeGIBakeSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "LightProbeGIBakeSystem shutdown!" << std::endl;
}

void LightProbeGIBakeSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("LightProbeGIBakeSystem");
#endif

	LightProbeGridComponent* probeGrid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	if (probeGrid == nullptr || !probeGrid->needBake) return;
	if (probeGrid->count.x + probeGrid->count.y + probeGrid->count.z <= 0) return;

	LightProbeGIBaking::resetProbes(gm);
	LightProbeGIBaking::bakeAll(gm);
	LightProbeGIBaking::bakeAll(gm);
	LightProbeGIBaking::bakeAll(gm);
	probeGrid->needBake = false;

	gm.forEachActiveEntity(
	    [&](Orhescyon::Entity entity)
	    {
		    if (auto* reflectionProbe = gm.getComponent<ReflectionProbeComponent>(entity))
			    reflectionProbe->needBake = true;
	    });
}
