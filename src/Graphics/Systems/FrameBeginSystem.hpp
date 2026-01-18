#pragma once

#include "../../Core/Systems/SystemContextual.hpp"

class FrameBeginSystem
    : public SystemContextual<FrameBeginSystem>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};