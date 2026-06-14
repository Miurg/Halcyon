#include "PhysSnapshotSystem.hpp"
#include <Jolt/Jolt.h>
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/PhysContexts.hpp"
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
		std::lock_guard<std::mutex> lock(_pendingMutex);
		_pendingAdd.push_back({entity, transSnapshot, physBody});
	}
}

void PhysSnapshotSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	std::lock_guard<std::mutex> lock(_pendingMutex);
	_pendingRemove.push_back(entity);
}

void PhysSnapshotSystem::applyPendingChanges()
{
	std::lock_guard<std::mutex> lock(_pendingMutex);

	if (!_pendingAdd.empty())
	{
		_agents.insert(_agents.end(), _pendingAdd.begin(), _pendingAdd.end());
		_pendingAdd.clear();
	}

	if (!_pendingRemove.empty())
	{
		for (Orhescyon::Entity entity : _pendingRemove)
		{
			auto it = std::remove_if(_agents.begin(), _agents.end(),
			                         [entity](const Agent& agent) { return agent.entity == entity; });
			_agents.erase(it, _agents.end());
		}
		_pendingRemove.clear();
	}
}

void PhysSnapshotSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("PhysSnapshotSystem");
#endif

	applyPendingChanges();

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
