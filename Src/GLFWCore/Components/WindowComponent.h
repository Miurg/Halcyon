#pragma once
#include "../../Core/Components/ComponentManager.h"
#include "../Window.h"
struct WindowComponent : Component
{
	Window* WindowInstance;
	WindowComponent(Window* w) : WindowInstance(w) {}
};