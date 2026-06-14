#pragma once

struct PhysTickRateComponent
{
	float rate = 60.0f;
	int maxConsecutiveMissedSteps = 5;
};
