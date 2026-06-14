#pragma once
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameBeginSystem.hpp"
#include "BufferUpdateSystem.hpp"

using Orhescyon::GeneralManager;
class LightUpdateSystem : public Orhescyon::SystemCore<LightUpdateSystem, GlobalTransformComponent, PointLightComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		GlobalTransformComponent* transform;
		PointLightComponent* lightInfo;
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
		return {typeid(GlobalTransformComponent), typeid(PointLightComponent), typeid(CurrentFrameComponent)};
	}
};