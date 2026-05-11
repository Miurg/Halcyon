#pragma once

#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

#include "../Components/PhysTransformSnapshot.hpp"
#include "../Components/PhysBodyComponent.hpp"
#include "../../Graphics/Systems/TransformSystem.hpp"
#include "PhysUpdateSystem.hpp"

using Orhescyon::GeneralManager;
class PhysSnapshotSystem : public Orhescyon::SystemCore<PhysSnapshotSystem, PhysTransformSnapshot, PhysBodyComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		PhysTransformSnapshot* transSnapshot;
		PhysBodyComponent* physBody;
	};

	std::vector<Agent> _agents;

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
		return {typeid(PhysTransformSnapshot)};
	}
	std::string_view getSystemManagerName() const override
	{
		return "physics";
	}
};