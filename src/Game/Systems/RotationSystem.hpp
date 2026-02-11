#pragma once

#include <vector>
#include "../../Core/Systems/SystemCore.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../../Core/Entitys/EntityManager.hpp"

class RotationSystem : public SystemCore<RotationSystem, LocalTransformComponent>
{
public:
	struct Agent
	{
		Entity entity;
		LocalTransformComponent* transform;
	};

	std::vector<Agent> _agents;

	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
};