#pragma once

#include "PlatformCore/Window.hpp"

struct WindowComponent
{
	Window* windowInstance;
	WindowComponent(Window* w) : windowInstance(w) {}
};