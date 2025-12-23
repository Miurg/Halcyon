#pragma once
struct KeyboardStateComponent
{
	bool Keys[1024] = {false};
	bool Mods[16] = {false};
};