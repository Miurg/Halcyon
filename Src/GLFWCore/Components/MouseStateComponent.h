#pragma once
#include "../../Core/Components/ComponentManager.h"
struct MouseStateComponent : Component
{
	bool Keys[32] = {false};
	bool Mods[16] = {false};
};