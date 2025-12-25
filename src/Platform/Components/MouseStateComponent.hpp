#pragma once
struct MouseStateComponent
{
	bool keys[32] = {false};
	bool mods[16] = {false};
};