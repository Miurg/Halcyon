#pragma once
struct MouseStateComponent
{
	bool Keys[32] = {false};
	bool Mods[16] = {false};
};