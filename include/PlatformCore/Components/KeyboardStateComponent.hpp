#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API KeyboardStateComponent
{
	bool keys[1024] = {false};
	bool mods[16] = {false};
};