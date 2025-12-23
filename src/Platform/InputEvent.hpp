#pragma once

struct InputEvent
{
	enum class Type
	{
		Key,
		MouseButton,
		MouseMove,
		MouseScroll,
		WindowResize
	};

	Type Type;

	int Key = 0;
	int Action = 0;
	int Mods = 0;

	// Для мыши
	double MousePositionX = 0.0, MousePositionY = 0.0;
	double DeltaScrollX = 0.0, DeltaScrollY = 0.0;
	unsigned int WindowWidth = 0, WindowHeight = 0;
};