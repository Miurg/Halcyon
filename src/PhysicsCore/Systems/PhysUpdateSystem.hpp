#pragma once

#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

#include "../Components/PhysBodyComponent.hpp"

using Orhescyon::GeneralManager;
class PhysUpdateSystem : public Orhescyon::SystemCore<PhysUpdateSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};