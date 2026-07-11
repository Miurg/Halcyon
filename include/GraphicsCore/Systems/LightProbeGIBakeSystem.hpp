#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

using Orhescyon::GeneralManager;

class HALCYON_API LightProbeGIBakeSystem : public Orhescyon::SystemCore<LightProbeGIBakeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};
