#include "PhysSyncSystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "../../PhysicsCore/PhysContexts.hpp"
#include "../../PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../../PhysicsCore/JoltGlm.hpp"

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
	PhysBodyComponent* physBody = gm.getComponent<PhysBodyComponent>(entity);

	if (transform && physBody)
	{
		_agents.push_back({entity, transform, physBody});
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
	JPH::PhysicsSystem& physicsSystem = *gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager->physicsSystem;
	JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

	for (auto& agent : _agents)
	{
		JPH::BodyID body = agent.physBody->bodyID;
		JPH::RVec3 physPosition = bi.GetCenterOfMassPosition(body);
		JPH::Quat physRotation = bi.GetRotation(body);

		GlobalTransformComponent& global = *agent.transform; 
		global.setGlobalPosition(toGlm(physPosition));
		global.setGlobalRotation(toGlm(physRotation));
	}
}