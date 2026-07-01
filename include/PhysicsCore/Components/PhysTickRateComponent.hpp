#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API PhysTickRateComponent
{
	float rate = 60.0f;
	int maxConsecutiveMissedSteps = 5;
};
