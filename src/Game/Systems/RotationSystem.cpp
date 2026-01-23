#include "RotationSystem.hpp"
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/GlobalTransformComponent.hpp"

void RotationSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RotationSystem registered!" << std::endl;
}

void RotationSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RotationSystem shutdown!" << std::endl;
}

void RotationSystem::update(float deltaTime, GeneralManager& gm)
{
	for (auto entity : entities)
	{
		LocalTransformComponent* transform = gm.getComponent<LocalTransformComponent>(entity);
		if (transform)
		{
			transform->rotateGlobal(glm::radians(5.0f) * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
		}
	}
}