#pragma once

#include "../../Orhescyon/Systems/SystemCore.hpp"
using Orhescyon::GeneralManager;
class DeltaTimeSystem : public Orhescyon::SystemCore<DeltaTimeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};