#pragma once

#include "../../Orhescyon/Systems/SystemCore.hpp"
using Orhescyon::GeneralManager;
class FrameBeginSystem : public Orhescyon::SystemCore<FrameBeginSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};