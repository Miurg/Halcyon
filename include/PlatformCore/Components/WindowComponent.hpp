#pragma once

#include "HalcyonExport.hpp"
#include "PlatformCore/Window.hpp"

struct HALCYON_API WindowComponent
{
	Window* windowInstance;
	WindowComponent(Window* w) : windowInstance(w) {}
};