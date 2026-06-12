#pragma once

#include "../Window.hpp"

struct WindowComponent
{
	Window* windowInstance;
	WindowComponent(Window* w) : windowInstance(w) {}
};