#pragma once
#include "../Window.hpp"
struct WindowComponent
{
	Window* WindowInstance;
	WindowComponent(Window* w) : WindowInstance(w) {}
};