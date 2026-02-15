#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32

class App
{
public:
	App();
	int run();
};
