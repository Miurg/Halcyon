#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API MouseStateComponent
{
	bool keys[32] = {false};
	bool mods[16] = {false};
};