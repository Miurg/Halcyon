#include "MainLoop.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Platform/Window.hpp"
void MainLoop::startLoop(GeneralManager& gm)
{
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	while (!window->shouldClose())
	{
		window->pollEvents();
		gm.update();
	}
}
