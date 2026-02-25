#include "RotationSystem.hpp"
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/GlobalTransformComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/DeltaTimeComponent.hpp"

void RotationSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RotationSystem registered!" << std::endl;
}

void RotationSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RotationSystem shutdown!" << std::endl;
}

void RotationSystem::onEntitySubscribed(Entity entity, GeneralManager& gm)
{
	LocalTransformComponent* transform = gm.getComponent<LocalTransformComponent>(entity);
	if (transform)
	{
		_agents.push_back({entity, transform});
	}
}

void RotationSystem::onEntityUnsubscribed(Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void RotationSystem::update(GeneralManager& gm)
{
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;
	for (auto& agent : _agents)
	{
		agent.transform->rotateGlobal(glm::radians(5.0f) * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
	}
}