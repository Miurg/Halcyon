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
	entities.push_back(entity);
}

void TransformSystem::updateTransformRecursive(Entity entity, GeneralManager& gm, glm::vec3 parentPosition,
                                               glm::quat parentRotation, glm::vec3 parentScale)
{
	glm::vec3 tempPosition = {0.0f, 0.0f, 0.0f};
	glm::quat tempRotation = {1.0f,0.0f, 0.0f, 0.0f};
	glm::vec3 tempScale = {1.0f, 1.0f, 1.0f};
	LocalTransformComponent& localTransform = 
		*gm.getComponent<LocalTransformComponent>(entity);
	GlobalTransformComponent& globalTransform = 
		*gm.getComponent<GlobalTransformComponent>(entity);

	// Scale 
	tempScale = parentScale * localTransform.localScale; 
	// Rotation 
	tempRotation = glm::normalize(parentRotation * localTransform.localRotation);
	// Position 
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
	
	for (auto entity : entities)
	{
		RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(entity);
		if (relationship->parent == NULL_ENTITY)
		{
			GlobalTransformComponent& globalTransform = *gm.getComponent<GlobalTransformComponent>(entity);
			updateTransformRecursive(entity, gm, tempPosition, tempRotation, tempScale);
		}
		
	}
}