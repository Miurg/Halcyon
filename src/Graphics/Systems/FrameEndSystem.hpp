#pragma once

#include "../../Core/Systems/SystemContextual.hpp"

class FrameEndSystem : public SystemContextual<FrameEndSystem>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};