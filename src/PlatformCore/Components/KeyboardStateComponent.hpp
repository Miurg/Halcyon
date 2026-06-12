#pragma once

struct KeyboardStateComponent
{
	bool keys[1024] = {false};
	bool mods[16] = {false};
};