#pragma once
#include "../../Core/Components/ComponentManager.h"
struct KeyboardStateComponent : Component
{
	bool Keys[1024] = {false};
	bool Mods[16] = {false};
};