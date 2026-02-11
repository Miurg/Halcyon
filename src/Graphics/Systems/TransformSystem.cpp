#include "TransformSystem.hpp"
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/RelationshipComponent.hpp"



void TransformSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "TransformSystem registered!" << std::endl;
}

void TransformSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "TransformSystem shutdown!" << std::endl;
}

void TransformSystem::onEntitySubscribed(Entity entity, GeneralManager& gm) 
{
	GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(entity);
	LocalTransformComponent* local = gm.getComponent<LocalTransformComponent>(entity);
	RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(entity);

	if (global && local && relationship)
	{
		_agents.push_back({entity, global, local, relationship});
	}
}

void TransformSystem::onEntityUnsubscribed(Entity entity, GeneralManager& gm)
{
	auto it = std::remove_if(_agents.begin(), _agents.end(),
	                         [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void TransformSystem::updateTransformRecursive(Entity entity, GeneralManager& gm, glm::vec3 parentPosition,
                                               glm::quat parentRotation, glm::vec3 parentScale)
{
	glm::vec3 tempPosition = {0.0f, 0.0f, 0.0f};
	glm::quat tempRotation = {1.0f,0.0f, 0.0f, 0.0f};
	glm::vec3 tempScale = {1.0f, 1.0f, 1.0f};
	// Note: Recursion still needs lookup because RelationshipComponent uses Entity IDs.
	// With Stable Storage, these lookups are safe but still incur sparse array access cost.
	LocalTransformComponent& localTransform = 
		*gm.getComponent<LocalTransformComponent>(entity);
	GlobalTransformComponent& globalTransform = 
		*gm.getComponent<GlobalTransformComponent>(entity);

	tempScale = parentScale * localTransform.localScale;  
	tempRotation = glm::normalize(parentRotation * localTransform.localRotation);
	tempPosition = parentPosition + (parentRotation * (parentScale * localTransform.localPosition));

	globalTransform.globalPosition = tempPosition;
	globalTransform.globalRotation = tempRotation;
	globalTransform.globalScale = tempScale;
	globalTransform.isModelDirty = true;
	globalTransform.isViewDirty = true;

	RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(entity);
	Entity currentChild = relationship->firstChild;
	while (currentChild != NULL_ENTITY)
	{
		updateTransformRecursive(currentChild, gm, tempPosition, tempRotation, tempScale);
		RelationshipComponent* siblingRel = gm.getComponent<RelationshipComponent>(currentChild);
		currentChild = siblingRel->nextSibling;
	}
}

void TransformSystem::update(float deltaTime, GeneralManager& gm) 
{
	glm::vec3 tempPosition = {0.0f, 0.0f, 0.0f};
	glm::quat tempRotation = {1.0f, 0.0f, 0.0f, 0.0f};
	glm::vec3 tempScale = {1.0f, 1.0f, 1.0f};
	
	for (auto& agent : _agents)
	{
		// Optimized root check using cached pointer
		if (agent.relationship->parent == NULL_ENTITY)
		{
			updateTransformRecursive(agent.entity, gm, tempPosition, tempRotation, tempScale);
		}
	}
}