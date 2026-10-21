#pragma once

#include <cstddef>
#include <vector>
#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"

using Orhescyon::GeneralManager;

/**
 * @brief Keeps local (parent-relative) and global (world-space) transforms in sync across the scene hierarchy,
 * resolving all edits once per frame.
 *
 * Transform setters on Local/GlobalTransformComponent never touch state directly: they only accumulate pending deltas
 * and raise dirty flags. This system is the single place where those deltas are resolved.
 *
 * Per entity, resolution is two-phase:
 *  - Phase 1 (global -> local): world-space edits are applied to the global transform, then local is back-computed from
 * it. For roots local == global.
 *  - Phase 2 (local -> global): local deltas are applied, then the final global is composed from the parent's global.
 *
 * Traversal is depth-first with an explicit stack: roots first, then each subtree parent-before-child, so the parent's
 * global is always final by the time its children compose against it.
 */

class HALCYON_API TransformSystem : public Orhescyon::SystemCore<TransformSystem, GlobalTransformComponent,
                                                                 LocalTransformComponent, RelationshipComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	struct StackItem
	{
		Orhescyon::Entity entity;
		bool isParentDirty;
	};

	struct CachedTransform
	{
		Orhescyon::Entity entity;
		GlobalTransformComponent* global;
		LocalTransformComponent* local;
		RelationshipComponent* relationship;
	};

	std::vector<StackItem> _nodeStack;
	std::vector<CachedTransform> _cachedTransforms;
	std::vector<std::size_t> _cachedTransformIndices;

	const CachedTransform* findCachedTransform(Orhescyon::Entity entity) const;
	static void applyPendingToLocal(LocalTransformComponent* local);
	static void applyPendingToGlobal(GlobalTransformComponent* global);
};
