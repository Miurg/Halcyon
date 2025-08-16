#pragma once
#include "../../Core/Components/ComponentManager.h"
struct CursorPositionComponent : Component
{
	double MousePositionX = 0.0, MousePositionY = 0.0;
};