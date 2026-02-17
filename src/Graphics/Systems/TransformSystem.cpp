#include "TransformSystem.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void TransformSystem::update(float deltaTime, GeneralManager& gm)
{
	struct StackItem
	{
		Entity entity;
		bool isParentDirty;
	};

	std::vector<StackItem> nodeStack;
	nodeStack.reserve(128); // Pre-reserve

	for (auto& agent : _agents)
	{
		if (agent.relationship->parent == NULL_ENTITY)
		{
			bool localDirty = agent.local->isModelDirty;

			if (localDirty)
			{
				agent.local->isModelDirty = false;

				agent.global->computeGlobalTransform(agent.local->localPosition, agent.local->localRotation,
				                                     agent.local->localScale);
				agent.global->isModelDirty = true;
				agent.global->isViewDirty = true;
			}

			if (agent.relationship->firstChild != NULL_ENTITY)
			{
				nodeStack.push_back({agent.relationship->firstChild, localDirty});
			}
		}
	}

	while (!nodeStack.empty())
	{
		StackItem currentItem = nodeStack.back();
		nodeStack.pop_back();

		Entity currentEntity = currentItem.entity;
		LocalTransformComponent* local = gm.getComponent<LocalTransformComponent>(currentEntity);
		GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(currentEntity);
		RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(currentEntity);

		bool localDirty = local->isModelDirty;
		if (localDirty)
		{
			local->isModelDirty = false; 
		}

		bool needsUpdate = localDirty || currentItem.isParentDirty;

		if (needsUpdate)
		{
			GlobalTransformComponent* parentGlobal = gm.getComponent<GlobalTransformComponent>(relationship->parent);
			if (parentGlobal)
			{
				glm::vec3 tempScale = parentGlobal->globalScale * local->localScale;
				glm::quat tempRotation = glm::normalize(parentGlobal->globalRotation * local->localRotation);
				glm::vec3 tempPosition =
				    parentGlobal->globalPosition +
				    (parentGlobal->globalRotation * (parentGlobal->globalScale * local->localPosition));

				global->computeGlobalTransform(tempPosition, tempRotation, tempScale);
				global->isModelDirty = true;
				global->isViewDirty = true;
			}
		}

		if (relationship->nextSibling != NULL_ENTITY)
		{
			nodeStack.push_back({relationship->nextSibling, currentItem.isParentDirty});
		}

		if (relationship->firstChild != NULL_ENTITY)
		{
			nodeStack.push_back({relationship->firstChild, needsUpdate});
		}
	}
}