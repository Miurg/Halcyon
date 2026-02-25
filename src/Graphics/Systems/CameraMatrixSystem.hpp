#pragma once
#include "../../Core/Systems/SystemCore.hpp"

class CameraMatrixSystem : public SystemCore<CameraMatrixSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};