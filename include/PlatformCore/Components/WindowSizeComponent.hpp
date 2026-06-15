#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API WindowSizeComponent
{
	unsigned int windowWidth = 0, windowHeight = 0;
	WindowSizeComponent(unsigned int width, unsigned int height) : windowWidth(width), windowHeight(height) {}
};