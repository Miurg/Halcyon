#pragma once

#include "../../PhysicsCore/Components/PhysTransformSnapshot.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameBeginSystem.hpp"
#include "BufferUpdateSystem.hpp"

using Orhescyon::GeneralManager;
class PhysSyncSystem : public Orhescyon::SystemCore<PhysSyncSystem, GlobalTransformComponent, PhysTransformSnapshot>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		GlobalTransformComponent* transform;
		PhysTransformSnapshot* physSnap;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(BufferUpdateSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(PhysTransformSnapshot)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(GlobalTransformComponent)};
	}
};