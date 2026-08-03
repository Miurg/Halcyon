#pragma once

#include "HalcyonExport.hpp"

struct HALCYON_API GodRaysSettingsComponent
{
	int raymarchStepCount = 16;
	float extinctionCoefficient = 0.02f;
	float scatteringCoefficient = 0.018f;
	float fogDensity = 0.01f;

	GodRaysSettingsComponent() = default;
};
