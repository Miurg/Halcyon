#pragma once

#include "../../Core/Systems/SystemCore.hpp"

class DeltaTimeSystem : public SystemCore<DeltaTimeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};