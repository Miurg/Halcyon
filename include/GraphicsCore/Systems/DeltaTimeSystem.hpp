#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
using Orhescyon::GeneralManager;

/**
 * @brief Measures the frame time and publishes it as the engine's single
 *        frame clock.
 *
 * Samples the window's high-resolution timer once per update and stores the delta since the previous sample in
 * DeltaTimeComponent, along with the accumulated totalTime.
 */

class HALCYON_API DeltaTimeSystem : public Orhescyon::SystemCore<DeltaTimeSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};
