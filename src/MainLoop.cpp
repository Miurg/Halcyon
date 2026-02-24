#include "MainLoop.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Platform/Window.hpp"
void MainLoop::startLoop(GeneralManager& gm)
{
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	while (!window->shouldClose())
	{
		float currentFrame = static_cast<float>(Window::getTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		window->pollEvents();
		gm.update(deltaTime);
	}
}
