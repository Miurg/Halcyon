#include "GraphicsCore/Systems/PhysSyncSystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "PhysicsCore/PhysContexts.hpp"
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "PhysicsCore/JoltGlm.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void PhysSyncSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "PhysSyncSystem registered!" << std::endl;
}

void PhysSyncSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "PhysSyncSystem shutdown!" << std::endl;
}

void PhysSyncSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("PhysSyncSystem");
#endif

	PhysManager& physManager = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;

	SnapshotIndices indicies = physManager.physSnapshot.load(std::memory_order_acquire);

	forEachSubscribedEntity(
	    gm,
	    [&](Orhescyon::Entity, GlobalTransformComponent& global, PhysTransformSnapshotComponent& physSnap)
	    {
		    JPH::RVec3 physPosition = physSnap.positionSnap[indicies.previous];
		    JPH::Quat physRotation = physSnap.rotationSnap[indicies.previous];

		    global.setGlobalPosition(toGlm(physPosition));
		    global.setGlobalRotation(toGlm(physRotation));
	    });
}