#pragma once

#include "../../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

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
};