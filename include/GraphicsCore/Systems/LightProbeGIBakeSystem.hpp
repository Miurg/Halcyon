#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Systems/RenderSystem.hpp"
#include "GraphicsCore/Systems/FrameEndSystem.hpp"

using Orhescyon::GeneralManager;

class HALCYON_API LightProbeGIBakeSystem : public Orhescyon::SystemCore<LightProbeGIBakeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(RenderSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameEndSystem)};
	}
};
