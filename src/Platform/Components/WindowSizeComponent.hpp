#pragma once
struct WindowSizeComponent
{
	unsigned int WindowWidth = 0, WindowHeight = 0;
	WindowSizeComponent(unsigned int width, unsigned int height) : WindowWidth(width), WindowHeight(height) {}
};