#include "MainLoop.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Platform/Window.hpp"
void MainLoop::startLoop(GeneralManager& gm)
{
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	std::atomic<bool> physRunning{true};
	std::jthread physThread(
	    [&]
	    {
		    while (physRunning.load(std::memory_order_relaxed))
		    {
			    gm.update("physics");
		    }
	    });

	while (!window->shouldClose())
	{
		window->pollEvents();
		gm.update();
	}

	physRunning.store(false, std::memory_order_relaxed);
	physThread.join();
}
