#pragma once
#include <queue>
#include "InputEvent.h"

struct GLFWwindow;

class Window
{
private:
	GLFWwindow* _GLFWwindow;

public:
	Window(unsigned int width, unsigned int height, const char* title);

	~Window();

	// Returns raw GLFW window handle.
	GLFWwindow* GetHandle() const;
	std::queue<InputEvent> InputQueue;

	bool ShouldClose() const;
	void PollEvents() const;
	void SwapBuffers() const;
	static void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow*, int button, int action, int mods);
	static void CursorPositionCallback(GLFWwindow*, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow*, double xoffset, double yoffset);
	static void FramebufferSizeCallback(GLFWwindow*, int width, int height);

	void HandleKey(int key, int scancode, int action, int mods);
	void HandleMouseButton(int button, int action, int mods);
	void HandleCursorPosition(double xpos, double ypos);
	void HandleScroll(double xoffset, double yoffset);
	void HandleFramebufferSize(int width, int height);
};