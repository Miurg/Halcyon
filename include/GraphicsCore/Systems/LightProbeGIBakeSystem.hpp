#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Systems/FrameBeginSystem.hpp"
#include "GraphicsCore/Systems/ReflectionProbeUpdateSystem.hpp"

using Orhescyon::GeneralManager;

class HALCYON_API LightProbeGIBakeSystem : public Orhescyon::SystemCore<LightProbeGIBakeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(ReflectionProbeUpdateSystem)};
	}
};
