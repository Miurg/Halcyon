#pragma once

#include "../../Core/Systems/SystemCore.hpp"

class FrameBeginSystem
    : public SystemCore<FrameBeginSystem>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};