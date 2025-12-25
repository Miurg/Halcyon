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

	int key = 0;
	int action = 0;
	int mods = 0;

	double mousePositionX = 0.0, mousePositionY = 0.0;
	double deltaScrollX = 0.0, deltaScrollY = 0.0;
	unsigned int windowWidth = 0, windowHeight = 0;
};