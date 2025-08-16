#pragma once
#include "../../Core/Components/ComponentManager.h"
struct WindowSizeComponent : Component
{
	unsigned int WindowWidth = 0, WindowHeight = 0;
	WindowSizeComponent(unsigned int width, unsigned int height) : WindowWidth(width), WindowHeight(height) {}
};