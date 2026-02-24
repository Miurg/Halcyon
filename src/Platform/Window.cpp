#include "Window.hpp"
#include <stdexcept>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

Window::Window(const char* title)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		throw std::runtime_error("glfwInit failed");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	int width = mode->width;
	int height = mode->height;

	_GLFWwindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (_GLFWwindow == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwSetWindowUserPointer(_GLFWwindow, this);
	glfwSetKeyCallback(getHandle(), keyCallback);
	glfwSetFramebufferSizeCallback(getHandle(), framebufferSizeCallback);
	glfwSetCursorPosCallback(getHandle(), cursorPositionCallback);
	glfwSetMouseButtonCallback(getHandle(), mouseButtonCallback);
	glfwSetScrollCallback(getHandle(), scrollCallback);
	this->width = width;
	this->height = height;
	glfwSetInputMode(_GLFWwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Window::~Window()
{
	if (_GLFWwindow)
	{
		glfwDestroyWindow(_GLFWwindow);
	}
	glfwTerminate();
}

GLFWwindow* Window::getHandle() const
{
	return _GLFWwindow;
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(_GLFWwindow);
}

void Window::setShouldClose(bool value)
{
	glfwSetWindowShouldClose(_GLFWwindow, value ? GLFW_TRUE : GLFW_FALSE);
}

void Window::pollEvents() const
{
	glfwPollEvents();
}

void Window::swapBuffers() const
{
	glfwSwapBuffers(_GLFWwindow);
}

std::vector<const char*> Window::getRequiredExtensions() const
{
	uint32_t count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);
	return std::vector<const char*>(extensions, extensions + count);
}

vk::SurfaceKHR Window::createSurface(vk::Instance instance) const
{
	VkSurfaceKHR c_surface;
	VkResult result = static_cast<VkResult>(glfwCreateWindowSurface(instance, _GLFWwindow, nullptr, &c_surface));

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}
	return vk::SurfaceKHR(c_surface);
}

void Window::waitEvents() const
{
	glfwWaitEvents();
}

// ===== Platform utility methods =====

double Window::getTime()
{
	return glfwGetTime();
}

void Window::setTime(double time)
{
	glfwSetTime(time);
}

void Window::setTitle(const char* title)
{
	glfwSetWindowTitle(_GLFWwindow, title);
}

void Window::setWindowSize(int w, int h)
{
	glfwSetWindowSize(_GLFWwindow, w, h);
}

void Window::getWindowSize(int* w, int* h) const
{
	glfwGetWindowSize(_GLFWwindow, w, h);
}

void Window::getFramebufferSize(int* w, int* h) const
{
	glfwGetFramebufferSize(_GLFWwindow, w, h);
}

bool Window::isFocused() const
{
	return glfwGetWindowAttrib(_GLFWwindow, GLFW_FOCUSED) == GLFW_TRUE;
}

bool Window::isMinimized() const
{
	return glfwGetWindowAttrib(_GLFWwindow, GLFW_ICONIFIED) == GLFW_TRUE;
}

void Window::setCursorMode(int mode)
{
	glfwSetInputMode(_GLFWwindow, GLFW_CURSOR, mode);
}

int Window::getCursorMode() const
{
	return glfwGetInputMode(_GLFWwindow, GLFW_CURSOR);
}

bool Window::isKeyPressed(int key) const
{
	return glfwGetKey(_GLFWwindow, key) == GLFW_PRESS;
}

bool Window::isMouseButtonPressed(int button) const
{
	return glfwGetMouseButton(_GLFWwindow, button) == GLFW_PRESS;
}

void Window::getCursorPos(double* x, double* y) const
{
	glfwGetCursorPos(_GLFWwindow, x, y);
}

void Window::setCursorPos(double x, double y)
{
	glfwSetCursorPos(_GLFWwindow, x, y);
}

void Window::getMonitorSize(int* w, int* h)
{
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	if (w) *w = mode->width;
	if (h) *h = mode->height;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self) self->handleKey(key, scancode, action, mods);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self) self->handleMouseButton(button, action, mods);
}

void Window::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self) self->handleCursorPosition(xpos, ypos);
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self) self->handleScroll(xoffset, yoffset);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self) self->handleFramebufferSize(width, height);
}

void Window::handleKey(int key, int scancode, int action, int mods)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::Key;
	ev.key = key;
	ev.action = action;
	ev.mods = mods;
	inputQueue.push(ev);
}
void Window::handleMouseButton(int button, int action, int mods)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseButton;
	ev.key = button;
	ev.action = action;
	ev.mods = mods;
	inputQueue.push(ev);
}
void Window::handleCursorPosition(double xpos, double ypos)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseMove;
	ev.mousePositionX = xpos;
	ev.mousePositionY = ypos;
	inputQueue.push(ev);
}
void Window::handleScroll(double xoffset, double yoffset)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseScroll;
	ev.deltaScrollX = xoffset;
	ev.deltaScrollY = yoffset;
	inputQueue.push(ev);
}
void Window::handleFramebufferSize(int width, int height)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::WindowResize;
	ev.windowWidth = width;
	ev.windowHeight = height;
	inputQueue.push(ev);

	this->width = width;
	this->height = height;
	framebufferResized = true;
}