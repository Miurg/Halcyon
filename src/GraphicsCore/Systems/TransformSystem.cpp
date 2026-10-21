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

		                        // Global -> local: apply global pending then sync with local.
		                        if (global._wasExternallyModified)
		                        {
			                        applyPendingToGlobal(&global);

			                        // Root: local == global in world space
			                        local.prs.position = global.prs.position;
			                        local.prs.rotation = global.prs.rotation;
			                        local.prs.scale = global.prs.scale;

			                        local._isModelDirty = true; // ensure phase 2 runs

			                        global._clearPending();
			                        dirty = true;
		                        }

		                        // Local -> global: apply local pending then sync back with global.
								// Also compute view staff only once here and sync with global.
		                        if (local._isModelDirty)
		                        {
			                        applyPendingToLocal(&local);

			                        global.prs.position = local.prs.position;
			                        global.prs.rotation = local.prs.rotation;
			                        global.prs.scale = local.prs.scale;

									global._updateDirectionVectors();
			                        global._isModelDirty = true;

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

		bool dirty = global->_wasExternallyModified || local->_isModelDirty || item.isParentDirty;

		if (dirty)
		{
			const CachedTransform* cachedParent = findCachedTransform(rel->parent);
			GlobalTransformComponent* parentGlobal =
			    cachedParent ? cachedParent->global : gm.getComponent<GlobalTransformComponent>(rel->parent);

			// Global -> local: apply global pending then sync with local (with info from parent).
			if (global->_wasExternallyModified)
			{
				applyPendingToGlobal(global);
				const glm::quat inverseParentRotation = glm::inverse(parentGlobal->prs.rotation);
				
				local->prs.scale = global->prs.scale / parentGlobal->prs.scale;
				local->prs.rotation = glm::normalize(inverseParentRotation * global->prs.rotation);
				local->prs.position = inverseParentRotation * ((global->prs.position - parentGlobal->prs.position) / parentGlobal->prs.scale);
				
				local->_isModelDirty = true;
				global->_clearPending();
			}


		    // Local -> global: apply local pending then sync back with global (again with info from parent).
			// Also compute view staff only once here and sync with global.
			if (local->_isModelDirty) applyPendingToLocal(local);

			// Even if local not dirty - still need to sync with parent changes.
			global->prs.scale = parentGlobal->prs.scale * local->prs.scale;
			global->prs.rotation = glm::normalize(parentGlobal->prs.rotation * local->prs.rotation);
			global->prs.position = parentGlobal->prs.position + (parentGlobal->prs.rotation * (parentGlobal->prs.scale * local->prs.position));
			
			global->_updateDirectionVectors();
			global->_isModelDirty = true;
			
			if (local->_isModelDirty) local->_clearPending();
		}

		// Depth first, so add child last to take it first.
		if (rel->nextSibling != NULL_ENTITY) _nodeStack.push_back({rel->nextSibling, item.isParentDirty});
		if (rel->firstChild != NULL_ENTITY) _nodeStack.push_back({rel->firstChild, dirty});
	}
}
