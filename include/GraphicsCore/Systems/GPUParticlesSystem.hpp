#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/ParticleEmitorComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API GPUParticlesSystem
    : public Orhescyon::SystemCore<GPUParticlesSystem, ParticleEmitorComponent, GlobalTransformComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	uint32_t numberOfEmiters;
	uint32_t numberOfParticles;
};