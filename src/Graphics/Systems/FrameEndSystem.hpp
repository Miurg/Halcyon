#pragma once

#include "../../Core/Systems/SystemCore.hpp"

class FrameEndSystem : public SystemCore<FrameEndSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};