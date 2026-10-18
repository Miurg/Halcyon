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

const TransformSystem::CachedTransform* TransformSystem::findCachedTransform(Orhescyon::Entity entity) const
{
	if (entity.slot >= _cachedTransformIndices.size()) return nullptr;

	// Indices persist across frames, so validate membership and entity generation.
	const std::size_t index = _cachedTransformIndices[entity.slot];
	if (index >= _cachedTransforms.size() || _cachedTransforms[index].entity != entity) return nullptr;

	return &_cachedTransforms[index];
}

void TransformSystem::applyPendingToLocal(LocalTransformComponent* local)
{
	local->prs.position += local->_pendingPositionDelta;
	local->prs.rotation = glm::normalize(local->prs.rotation * local->_pendingRotationDelta);
	local->prs.scale += local->_pendingScaleDelta;
	local->_updateDirectionVectors();
}

void TransformSystem::applyPendingToGlobal(GlobalTransformComponent* global)
{
	global->prs.position += global->_pendingPositionDelta;
	global->prs.rotation = glm::normalize(global->prs.rotation * global->_pendingRotationDelta);
	global->prs.scale += global->_pendingScaleDelta;
	// Phase 2 refreshes direction vectors from the final rotation after local changes.
}

void TransformSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("TransformSystem");
#endif

	_nodeStack.clear();
	_nodeStack.reserve(128);
	_cachedTransforms.clear();

	// === Root entities ===

	forEachSubscribedEntity(gm,
	                        [&](Orhescyon::Entity entity, GlobalTransformComponent& global,
	                            LocalTransformComponent& local, RelationshipComponent& relationship)
	                        {
		                        const bool isRoot = relationship.parent == NULL_ENTITY;
		                        if (!isRoot || relationship.firstChild != NULL_ENTITY)
		                        {
			                        const std::size_t slot = entity.slot;
			                        if (slot >= _cachedTransformIndices.size()) _cachedTransformIndices.resize(slot + 1);
			                        _cachedTransformIndices[slot] = _cachedTransforms.size();
			                        _cachedTransforms.push_back({entity, &global, &local, &relationship});
		                        }

		                        if (!isRoot) return;

		                        bool dirty = false;

		                        // Phase 1: apply global pending → sync down into local
		                        if (global._wasExternallyModified)
		                        {
			                        applyPendingToGlobal(&global);
			                        // Root: local == global in world space
			                        local.prs.position = global.prs.position;
			                        local.prs.rotation = global.prs.rotation;
			                        local.prs.scale = global.prs.scale;
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
			                        global._front = local._front;
			                        global._up = local._up;
			                        global._right = local._right;
			                        global._isModelDirty = true;
			                        global._isViewDirty = true;
			                        local._clearPending();
			                        dirty = true;
		                        }

		                        if (relationship.firstChild != NULL_ENTITY)
			                        _nodeStack.push_back({relationship.firstChild, dirty});
	                        });

	// === Child entities (depth-first) ===

	while (!_nodeStack.empty())
	{
		const StackItem item = _nodeStack.back();
		_nodeStack.pop_back();

		const CachedTransform* cached = findCachedTransform(item.entity);
		LocalTransformComponent* local = cached ? cached->local : gm.getComponent<LocalTransformComponent>(item.entity);
		GlobalTransformComponent* global =
		    cached ? cached->global : gm.getComponent<GlobalTransformComponent>(item.entity);
		RelationshipComponent* rel = cached ? cached->relationship : gm.getComponent<RelationshipComponent>(item.entity);

		const bool dirty = global->_wasExternallyModified || local->_isModelDirty || item.isParentDirty;
		if (dirty)
		{
			const CachedTransform* cachedParent = findCachedTransform(rel->parent);
			GlobalTransformComponent* pg =
			    cachedParent ? cachedParent->global : gm.getComponent<GlobalTransformComponent>(rel->parent);

			// Phase 1: apply global pending -> back-compute local from new global
			if (global->_wasExternallyModified)
			{
				applyPendingToGlobal(global);
				const glm::quat inverseParentRotation = glm::inverse(pg->prs.rotation);
				local->prs.scale = global->prs.scale / pg->prs.scale;
				local->prs.rotation = glm::normalize(inverseParentRotation * global->prs.rotation);
				local->prs.position = inverseParentRotation * ((global->prs.position - pg->prs.position) / pg->prs.scale);
				global->_clearPending();
				local->_isModelDirty = true;
			}

			// Phase 2: apply local pending -> propagate local -> global
			if (local->_isModelDirty) applyPendingToLocal(local);

			global->prs.scale = pg->prs.scale * local->prs.scale;
			global->prs.rotation = glm::normalize(pg->prs.rotation * local->prs.rotation);
			global->prs.position = pg->prs.position + (pg->prs.rotation * (pg->prs.scale * local->prs.position));
			global->_updateDirectionVectors();
			global->_isModelDirty = true;
			global->_isViewDirty = true;
			local->_clearPending();
		}

		if (rel->nextSibling != NULL_ENTITY) _nodeStack.push_back({rel->nextSibling, item.isParentDirty});
		if (rel->firstChild != NULL_ENTITY) _nodeStack.push_back({rel->firstChild, dirty});
	}
}
