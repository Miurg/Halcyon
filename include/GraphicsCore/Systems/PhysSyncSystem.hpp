#pragma once

#include "HalcyonExport.hpp"
#include "PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

using Orhescyon::GeneralManager;
class HALCYON_API PhysSyncSystem : public Orhescyon::SystemCore<PhysSyncSystem, GlobalTransformComponent, PhysTransformSnapshotComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};