#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API DeltaTimeComponent
{
	float deltaTime = 0.0f;
	float totalTime = 0.0f;
	float lastFrameTime = 0.0f;
};
