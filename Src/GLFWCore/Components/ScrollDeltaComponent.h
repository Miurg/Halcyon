#pragma once
#include "../../Core/Components/ComponentManager.h"
struct ScrollDeltaComponent : Component
{
	double DeltaScrollX = 0.0, DeltaScrollY = 0.0;
};