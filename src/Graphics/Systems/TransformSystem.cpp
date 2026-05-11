#include "TransformSystem.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../GraphicsContexts.hpp"
#include "../Components/RelationshipComponent.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void TransformSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "TransformSystem registered!" << std::endl;
}

void TransformSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "TransformSystem shutdown!" << std::endl;
}

void TransformSystem::onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(entity);
	LocalTransformComponent* local = gm.getComponent<LocalTransformComponent>(entity);
	RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(entity);

	if (global && local && relationship)
	{
		_agents.push_back({entity, global, local, relationship});
	}
}

void TransformSystem::onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm)
{
	auto it =
	    std::remove_if(_agents.begin(), _agents.end(), [entity](const Agent& agent) { return agent.entity == entity; });
	_agents.erase(it, _agents.end());
}

void TransformSystem::applyPendingToLocal(LocalTransformComponent* local)
{
	local->_localPosition += local->_pendingPositionDelta;
	local->_localRotation = glm::normalize(local->_localRotation * local->_pendingRotationDelta);
	if (local->_hasPendingScale) local->_localScale = local->_pendingScale;
	local->_updateDirectionVectors();
}

void TransformSystem::applyPendingToGlobal(GlobalTransformComponent* global)
{
	global->_globalPosition += global->_pendingPositionDelta;
	global->_globalRotation = glm::normalize(global->_globalRotation * global->_pendingRotationDelta);
	if (global->_hasPendingScale) global->_globalScale = global->_pendingScale;
	global->_updateDirectionVectors();
}

void TransformSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("TransformSystem");
#endif

	struct StackItem
	{
		Orhescyon::Entity entity;
		bool isParentDirty;
	};

	std::vector<StackItem> nodeStack;
	nodeStack.reserve(128);

	// === Root entities ===

	for (auto& agent : _agents)
	{
		if (agent.relationship->parent != NULL_ENTITY) continue;

		bool dirty = false;

		// Phase 1: apply global pending → sync down into local
		if (agent.global->_wasExternallyModified)
		{
			applyPendingToGlobal(agent.global);
			// Root: local == global in world space
			agent.local->_localPosition = agent.global->_globalPosition;
			agent.local->_localRotation = agent.global->_globalRotation;
			agent.local->_localScale = agent.global->_globalScale;
			agent.local->_updateDirectionVectors();
			agent.global->_clearPending();
			agent.local->_isModelDirty = true; // ensure phase 2 runs
			dirty = true;
		}

		// Phase 2: apply local pending → push up into global
		if (agent.local->_isModelDirty)
		{
			applyPendingToLocal(agent.local);
			agent.global->_globalPosition = agent.local->_localPosition;
			agent.global->_globalRotation = agent.local->_localRotation;
			agent.global->_globalScale = agent.local->_localScale;
			agent.global->_updateDirectionVectors();
			agent.global->_isModelDirty = true;
			agent.global->_isViewDirty = true;
			agent.local->_clearPending();
			dirty = true;
		}

		if (agent.relationship->firstChild != NULL_ENTITY) nodeStack.push_back({agent.relationship->firstChild, dirty});
	}

	// === Child entities (depth-first) ===

	while (!nodeStack.empty())
	{
		StackItem item = nodeStack.back();
		nodeStack.pop_back();

		LocalTransformComponent* local = gm.getComponent<LocalTransformComponent>(item.entity);
		GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(item.entity);
		RelationshipComponent* rel = gm.getComponent<RelationshipComponent>(item.entity);
		GlobalTransformComponent* pg = gm.getComponent<GlobalTransformComponent>(rel->parent);

		bool dirty = false;

		// Phase 1: apply global pending -> back-compute local from new global
		if (global->_wasExternallyModified)
		{
			applyPendingToGlobal(global);
			local->_localScale = global->_globalScale / pg->_globalScale;
			local->_localRotation = glm::normalize(glm::inverse(pg->_globalRotation) * global->_globalRotation);
			local->_localPosition =
			    glm::inverse(pg->_globalRotation) * ((global->_globalPosition - pg->_globalPosition) / pg->_globalScale);
			local->_updateDirectionVectors();
			global->_clearPending();
			local->_isModelDirty = true;
			dirty = true;
		}

		bool needsUpdate = local->_isModelDirty || item.isParentDirty;

		// Phase 2: apply local pending -> propagate local -> global
		if (needsUpdate)
		{
			if (local->_isModelDirty) applyPendingToLocal(local);

			global->_globalScale = pg->_globalScale * local->_localScale;
			global->_globalRotation = glm::normalize(pg->_globalRotation * local->_localRotation);
			global->_globalPosition =
			    pg->_globalPosition + (pg->_globalRotation * (pg->_globalScale * local->_localPosition));
			global->_updateDirectionVectors();
			global->_isModelDirty = true;
			global->_isViewDirty = true;
			local->_clearPending();
			dirty = true;
		}

		if (rel->nextSibling != NULL_ENTITY) nodeStack.push_back({rel->nextSibling, item.isParentDirty});
		if (rel->firstChild != NULL_ENTITY) nodeStack.push_back({rel->firstChild, dirty});
	}
}
