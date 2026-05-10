#include "PhysUpdateSystem.hpp"
#include "../PhysContexts.hpp"
#include "../Components/PhysManagerComponent.hpp"
#include "../Components/PhysTickRateComponent.hpp"
#include <Jolt/Jolt.h>
#include "../../Platform/Components/DeltaTimeComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void PhysUpdateSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "PhysicsUpdateSystem registered!" << std::endl;
}

void PhysUpdateSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "PhysicsUpdateSystem shutdown!" << std::endl;
}

void PhysUpdateSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("PhysUpdateSystem");
#endif

	const float deltaTime =
	    1.0f / gm.getContextComponent<PhysTickRateContext, PhysTickRateComponent>()->rate;
	JPH::PhysicsSystem* physicsSystem =
	    gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager->physicsSystem;
	JPH::TempAllocatorImpl* tempAllocator =
	    gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager->tempAllocator;
	JPH::JobSystemThreadPool* jobSystem =
	    gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager->jobSystem;
	physicsSystem->Update(deltaTime, 1, tempAllocator, jobSystem);
}
