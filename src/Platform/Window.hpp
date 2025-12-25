#pragma once
#include <queue>
#include <vector>
#include "InputEvent.hpp"
#include <vulkan/vulkan.hpp>

struct GLFWwindow;

class Window
{
private:
	GLFWwindow* _GLFWwindow;

public:
	Window(const char* title);

	~Window();

	// Returns raw GLFW window handle.
	GLFWwindow* getHandle() const;
	std::queue<InputEvent> InputQueue;

	bool shouldClose() const;
	void pollEvents() const;
	void swapBuffers() const;
	int width;
	int height;
	bool framebufferResized = false;
	std::vector<const char*> getRequiredExtensions() const;
	vk::SurfaceKHR createSurface(vk::Instance instance) const;
	void waitEvents() const;

	static void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow*, int button, int action, int mods);
	static void cursorPositionCallback(GLFWwindow*, double xpos, double ypos);
	static void scrollCallback(GLFWwindow*, double xoffset, double yoffset);
	static void framebufferSizeCallback(GLFWwindow*, int width, int height);

	void handleKey(int key, int scancode, int action, int mods);
	void handleMouseButton(int button, int action, int mods);
	void handleCursorPosition(double xpos, double ypos);
	void handleScroll(double xoffset, double yoffset);
	void handleFramebufferSize(int width, int height);
};