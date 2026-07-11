#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API TransformSystem : public Orhescyon::SystemCore<TransformSystem, GlobalTransformComponent, LocalTransformComponent,
                                                   RelationshipComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	static void applyPendingToLocal(LocalTransformComponent* local);
	static void applyPendingToGlobal(GlobalTransformComponent* global);
};