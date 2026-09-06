#include "GraphicsCore/Systems/TransformSystem.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"

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

void TransformSystem::applyPendingToLocal(LocalTransformComponent* local)
{
	local->prs.position += local->_pendingPositionDelta;
	local->prs.rotation = glm::normalize(local->prs.rotation * local->_pendingRotationDelta);
	if (local->_hasPendingScale) local->prs.scale = local->_pendingScale;
	local->_updateDirectionVectors();
}

void TransformSystem::applyPendingToGlobal(GlobalTransformComponent* global)
{
	global->prs.position += global->_pendingPositionDelta;
	global->prs.rotation = glm::normalize(global->prs.rotation * global->_pendingRotationDelta);
	if (global->_hasPendingScale) global->prs.scale = global->_pendingScale;
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

	forEachSubscribedEntity(
	    gm,
	    [&](Orhescyon::Entity entity, GlobalTransformComponent& global, LocalTransformComponent& local,
	        RelationshipComponent& relationship)
	    {
		    if (relationship.parent != NULL_ENTITY) return;

		    bool dirty = false;

		    // Phase 1: apply global pending → sync down into local
		    if (global._wasExternallyModified)
		    {
			    applyPendingToGlobal(&global);
			    // Root: local == global in world space
			    local.prs.position = global.prs.position;
			    local.prs.rotation = global.prs.rotation;
			    local.prs.scale = global.prs.scale;
			    local._updateDirectionVectors();
			    global._clearPending();
			    local._isModelDirty = true; // ensure phase 2 runs
			    dirty = true;
		    }

		    // Phase 2: apply local pending → push up into global
		    if (local._isModelDirty)
		    {
			    applyPendingToLocal(&local);
			    global.prs.position = local.prs.position;
			    global.prs.rotation = local.prs.rotation;
			    global.prs.scale = local.prs.scale;
			    global._updateDirectionVectors();
			    global._isModelDirty = true;
			    global._isViewDirty = true;
			    local._clearPending();
			    dirty = true;
		    }

		    if (relationship.firstChild != NULL_ENTITY) nodeStack.push_back({relationship.firstChild, dirty});
	    });

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
			local->prs.scale = global->prs.scale / pg->prs.scale;
			local->prs.rotation = glm::normalize(glm::inverse(pg->prs.rotation) * global->prs.rotation);
			local->prs.position =
			    glm::inverse(pg->prs.rotation) * ((global->prs.position - pg->prs.position) / pg->prs.scale);
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

			global->prs.scale = pg->prs.scale * local->prs.scale;
			global->prs.rotation = glm::normalize(pg->prs.rotation * local->prs.rotation);
			global->prs.position = pg->prs.position + (pg->prs.rotation * (pg->prs.scale * local->prs.position));
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
