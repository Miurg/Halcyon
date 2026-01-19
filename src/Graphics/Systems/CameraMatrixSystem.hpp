#pragma once
#include "../../Core/Systems/SystemContextual.hpp"

class CameraMatrixSystem : public SystemContextual<CameraMatrixSystem>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};