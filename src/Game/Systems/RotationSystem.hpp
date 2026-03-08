#pragma once

#include <vector>
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

using Orhescyon::GeneralManager;

class RotationSystem : public Orhescyon::SystemCore<RotationSystem, LocalTransformComponent>
{
public:
	struct Agent
	{
		Entity entity;
		LocalTransformComponent* transform;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
};