#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

Window::Window(unsigned int width, unsigned int height, const char* title)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		throw std::runtime_error("glfwInit failed");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (m_window == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwMakeContextCurrent(m_window);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwDestroyWindow(m_window);
		glfwTerminate();
		throw std::runtime_error("gladLoadGLLoader failed");
	}

	// Set initial viewport size.
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);
}

Window::~Window()
{
	if (m_window)
	{
		glfwDestroyWindow(m_window);
	}
	glfwTerminate();
}

GLFWwindow* Window::GetHandle() const
{
	return m_window;
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::SwapBuffers() const
{
	glfwSwapBuffers(m_window);
}