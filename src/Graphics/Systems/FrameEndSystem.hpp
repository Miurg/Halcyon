#pragma once

#include "../../Orhescyon/Systems/SystemCore.hpp"
using Orhescyon::GeneralManager;
class FrameEndSystem : public Orhescyon::SystemCore<FrameEndSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};