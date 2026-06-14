#pragma once

#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <HalcyonGraphics.hpp>

using Orhescyon::GeneralManager;
class SpawnSystem : public Orhescyon::SystemCore<SpawnSystem>
{

public:
	int j = 0;
	int k = 0;
	float time = 0;
	std::vector<Orhescyon::Entity> entities;
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override
	{
		entities.push_back(entity);
	}
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override
	{
		entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
	}
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
};
