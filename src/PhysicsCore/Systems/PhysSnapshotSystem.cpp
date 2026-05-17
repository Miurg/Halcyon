#include "PhysSnapshotSystem.hpp"
#include <Jolt/Jolt.h>
#include "../Components/PhysManagerComponent.hpp"
#include "../PhysContexts.hpp"
#include "../Managers/PhysManager.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void PhysSnapshotSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "PhysSnapshotSystem registered!" << std::endl;
}

void PhysSnapshotSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "PhysSnapshotSystem shutdown!" << std::endl;
}

void PhysSnapshotSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	PhysBodyComponent* physBody = gm.getComponent<PhysBodyComponent>(entity);
	PhysTransformSnapshotComponent* transSnapshot = gm.getComponent<PhysTransformSnapshotComponent>(entity);
	
	if (transSnapshot && physBody)
	{
		_agents.push_back({entity, transSnapshot, physBody});
	}
}

void PhysSnapshotSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void PhysSnapshotSystem::update(GeneralManager& gm) 
{
#ifdef TRACY_ENABLE
	ZoneScopedN("PhysSnapshotSystem");
#endif

	PhysManager& physManager =
	    *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;
	JPH::BodyInterface& bi = physManager.physicsSystem->GetBodyInterface();

	SnapshotIndices indicies = physManager.physSnapshot.load(std::memory_order_relaxed);

	for (auto& agent : _agents)
	{
		JPH::BodyID body = agent.physBody->bodyID;
		PhysTransformSnapshotComponent& snap = *agent.transSnapshot;
		snap.positionSnap[indicies.writing] = bi.GetCenterOfMassPosition(body);
		snap.rotationSnap[indicies.writing] = bi.GetRotation(body);
	}

	indicies.current = (indicies.current + 1) % maxSnapshots;
	indicies.previous = (indicies.previous + 1) % maxSnapshots;
	indicies.writing = (indicies.writing + 1) % maxSnapshots;

	physManager.physSnapshot.store(indicies, std::memory_order_release);
}
