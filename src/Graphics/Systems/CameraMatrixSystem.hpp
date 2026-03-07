#pragma once
#include "../../Orhescyon/Systems/SystemCore.hpp"
using Orhescyon::GeneralManager;
class CameraMatrixSystem : public Orhescyon::SystemCore<CameraMatrixSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};