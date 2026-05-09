#include "PhysSyncSystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "../../PhysicsCore/PhysContexts.hpp"
#include "../../PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../../PhysicsCore/JoltGlm.hpp"

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

void PhysSyncSystem::onEntitySubscribed(Entity entity, GeneralManager& gm)
{
	GlobalTransformComponent* transform = gm.getComponent<GlobalTransformComponent>(entity);
	PhysTransformSnapshot* physSnap = gm.getComponent<PhysTransformSnapshot>(entity);

	if (transform && physSnap)
	{
		_agents.push_back({entity, transform, physSnap});
	}
}

void PhysSyncSystem::onEntityUnsubscribed(Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void PhysSyncSystem::update(GeneralManager& gm) 
{
#ifdef TRACY_ENABLE
	ZoneScopedN("PhysSyncSystem");
#endif

	PhysManager& physManager = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;

	SnapshotIndices indicies = physManager.physSnapshot.load(std::memory_order_acquire);

	for (auto& agent : _agents)
	{
		PhysTransformSnapshot* physSnap = agent.physSnap;
		JPH::RVec3 physPosition = physSnap->positionSnap[indicies.previous];
		JPH::Quat physRotation = physSnap->rotationSnap[indicies.previous];

		GlobalTransformComponent& global = *agent.transform; 
		global.setGlobalPosition(toGlm(physPosition));
		global.setGlobalRotation(toGlm(physRotation));
	}
}