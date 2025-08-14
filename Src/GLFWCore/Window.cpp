#include "Window.h"

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

	_GLFWwindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (_GLFWwindow == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwMakeContextCurrent(_GLFWwindow);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwDestroyWindow(_GLFWwindow);
		glfwTerminate();
		throw std::runtime_error("gladLoadGLLoader failed");
	}

	// Set initial viewport size.
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(_GLFWwindow, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);
}

Window::~Window()
{
	if (_GLFWwindow)
	{
		glfwDestroyWindow(_GLFWwindow);
	}
	glfwTerminate();
}

GLFWwindow* Window::GetHandle() const
{
	return _GLFWwindow;
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(_GLFWwindow);
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::SwapBuffers() const
{
	glfwSwapBuffers(_GLFWwindow);
}

void Window::KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::Key;
	ev.Key = key;
	ev.Action = action;
	ev.Mods = mods;
	InputQueue.push(ev);
}

void Window::MouseButtonCallback(GLFWwindow*, int button, int action, int mods)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseButton;
	ev.Key = button;
	ev.Action = action;
	ev.Mods = mods;
	InputQueue.push(ev);
}

void Window::CursorPositionCallback(GLFWwindow*, double xpos, double ypos)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseMove;
	ev.MousePositionX = xpos;
	ev.MousePositionY = ypos;
	InputQueue.push(ev);
}

void Window::ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::MouseScroll;
	ev.DeltaScrollX = xoffset;
	ev.DeltaScrollY = yoffset;
	InputQueue.push(ev);
}

void Window::FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
	InputEvent ev;
	ev.Type = InputEvent::Type::WindowResize;
	ev.WindowWidth = width;
	ev.WindowHeight = height;
	InputQueue.push(ev);
}