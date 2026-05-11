#pragma once

#include <vector>
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/DeltaTimeComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../../Graphics/Systems/DeltaTimeSystem.hpp"
#include "../../Graphics/Systems/FrameBeginSystem.hpp"

using Orhescyon::GeneralManager;

class RotationSystem : public Orhescyon::SystemCore<RotationSystem, LocalTransformComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		LocalTransformComponent* transform;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(DeltaTimeSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(DeltaTimeComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(LocalTransformComponent)};
	}
};