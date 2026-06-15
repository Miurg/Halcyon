#pragma once

#include <vector>
#include <mutex>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

#include "PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"
#include "PhysicsCore/Systems/PhysUpdateSystem.hpp"

using Orhescyon::GeneralManager;
class PhysSnapshotSystem
    : public Orhescyon::SystemCore<PhysSnapshotSystem, PhysTransformSnapshotComponent, PhysBodyComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		PhysTransformSnapshotComponent* transSnapshot;
		PhysBodyComponent* physBody;
	};

	std::vector<Agent> _agents;

	std::mutex _pendingMutex;
	std::vector<Agent> _pendingAdd;
	std::vector<Orhescyon::Entity> _pendingRemove;

	void applyPendingChanges();

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(PhysUpdateSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(PhysBodyComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(PhysTransformSnapshotComponent)};
	}
	std::string_view getSystemManagerName() const override
	{
		return "physics";
	}
};