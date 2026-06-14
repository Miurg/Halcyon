#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/ParticleEmitorComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "FrameBeginSystem.hpp"
#include "FrameEndSystem.hpp"

using Orhescyon::GeneralManager;
class GPUParticlesSystem
    : public Orhescyon::SystemCore<GPUParticlesSystem, ParticleEmitorComponent, GlobalTransformComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		ParticleEmitorComponent* particleEmitor;
		GlobalTransformComponent* transform;
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
		return {typeid(FrameEndSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(ParticleEmitorComponent), typeid(GlobalTransformComponent)};
	}

private:
	uint32_t numberOfEmiters;
	uint32_t numberOfParticles;
};