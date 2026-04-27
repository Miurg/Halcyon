#pragma once

#include "../../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameBeginSystem.hpp"
#include "BufferUpdateSystem.hpp"

using Orhescyon::GeneralManager;
class PhysSyncSystem : public Orhescyon::SystemCore<PhysSyncSystem, GlobalTransformComponent, PhysBodyComponent>
{
public:
	struct Agent
	{
		Entity entity;
		GlobalTransformComponent* transform;
		PhysBodyComponent* physBody;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
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
		return {typeid(PhysBodyComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(GlobalTransformComponent)};
	}
};