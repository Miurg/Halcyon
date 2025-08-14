#pragma once
#include <queue>
#include "InputEvent.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

struct GLFWwindow; // forward declaration to avoid including glfw3.h here

class Window
{
private:
	GLFWwindow* _GLFWwindow;

public:
	// Creates a window, initializes GLFW/GLAD and sets the OpenGL context.
	Window(unsigned int width, unsigned int height, const char* title);

	~Window();

	// Returns raw GLFW window handle.
	GLFWwindow* GetHandle() const;
	std::queue<InputEvent> InputQueue;

	// Convenience wrappers.
	bool ShouldClose() const;
	void PollEvents() const;
	void SwapBuffers() const;
	void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
	void MouseButtonCallback(GLFWwindow*, int button, int action, int mods);
	void CursorPositionCallback(GLFWwindow*, double xpos, double ypos);
	void ScrollCallback(GLFWwindow*, double xoffset, double yoffset);
	void FramebufferSizeCallback(GLFWwindow*, int width, int height);
};