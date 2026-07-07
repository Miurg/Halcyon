#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"

using Orhescyon::GeneralManager;

class ReflectionProbeBaker
{
public:
	// Bakes one probe
	static void bake(GeneralManager& gm, ReflectionProbeComponent& probe, int cubemapSlot);
};
