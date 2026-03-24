#pragma once
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/PointLightComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

using Orhescyon::GeneralManager;
class LightUpdateSystem : public Orhescyon::SystemCore<LightUpdateSystem, GlobalTransformComponent, PointLightComponent>
{
public:
	struct Agent
	{
		Entity entity;
		GlobalTransformComponent* transform;
		PointLightComponent* lightInfo;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
};