#include "PhysicsCleanup.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include "Components/PhysManagerComponent.hpp"
#include "PhysContexts.hpp"

void PhysicsCleanup::cleanup(GeneralManager& gm)
{
	PhysManagerComponent* physManagerComp = gm.getContextComponent<PhysManagerContext, PhysManagerComponent>();
	if (physManagerComp && physManagerComp->physManager)
	{
		delete physManagerComp->physManager;
		physManagerComp->physManager = nullptr;
	}

	JPH::UnregisterTypes();

	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}
