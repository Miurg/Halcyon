#pragma once

struct GLFWwindow; // forward declaration to avoid including glfw3.h here

class Window
{
public:
	// Creates a window, initializes GLFW/GLAD and sets the OpenGL context.
	Window(unsigned int width, unsigned int height, const char* title);

	~Window();

	// Returns raw GLFW window handle.
	GLFWwindow* GetHandle() const;

	// Convenience wrappers.
	bool ShouldClose() const;
	void PollEvents() const;
	void SwapBuffers() const;

private:
	GLFWwindow* m_window;
};